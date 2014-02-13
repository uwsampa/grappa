
########################################################################
## This file is part of Grappa, a system for scaling irregular
## applications on commodity clusters. 

## Copyright (C) 2010-2014 University of Washington and Battelle
## Memorial Institute. University of Washington authorizes use of this
## Grappa software.

## Grappa is free software: you can redistribute it and/or modify it
## under the terms of the Affero General Public License as published
## by Affero, Inc., either version 1 of the License, or (at your
## option) any later version.

## Grappa is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## Affero General Public License for more details.

## You should have received a copy of the Affero General Public
## License along with this program. If not, you may obtain one from
## http://www.affero.org/oagpl.html.
########################################################################

require 'igor'

# add some Grappa-specific things to Igor so they're available at the prompt
module Igor
  def sq
    case `hostname`
    when /pal/
      puts `squeue -ppal -o '%.7i %.4P %.17j %.8u %.2t %.10M %.6D %R'`
    else
      puts `squeue`
    end
  end
end

module Isolatable
  extend self # (so all methods become class methods, like Igor module)
  
  def self.included(base) # do stuff when 'include Isolate' is called
    base.extend(self) # add all of this module's definitions to base module
  end
  
  def isolate(exes, shared_dir=nil)
    puts "########## Isolating ##########"
    @isolate_called = true

    # set aside copy of executable and its libraries
    # ldir = "/scratch/#{ENV['USER']}/igor/#{Process.pid}"
    script_dir = File.dirname(caller[0][/(^.*?):/,1]) # if symlink, this is location of symlink
    shared_dir = script_dir unless shared_dir
    @ldir = "#{File.expand_path shared_dir}/.igor/#{Process.pid}"
    puts "making #{@ldir}"
    FileUtils.mkdir_p(@ldir)
    
    exes = [exes] unless exes.is_a? Array
    exes << 'mpirun' << "#{File.dirname(__FILE__)}/../bin/grappa_srun" \
                     << "#{File.dirname(__FILE__)}/../bin/srun_prolog.rb" \
                     << "#{File.dirname(__FILE__)}/../bin/srun_epilog.sh"
    
    exes.each do |exe|
      if not File.exists? exe
        if File.exists? "#{script_dir}/#{exe}"
          exe = "#{script_dir}/#{exe}"
        else
          exe = `which #{exe}`.strip          
        end
      end
      FileUtils.cp(exe, @ldir)
      libs = `bash -c "LD_LIBRARY_PATH=#{$GRAPPA_LIBPATH} ldd #{exe}"`
                .split(/\n/)
                .map{|s| s[/.*> (.*) \(.*/,1] }
                .select{|s| s != nil and s != "" and
                  not(s[/linux-vdso\.so|ld-linux|^\/lib64/]) }
                .each {|l| FileUtils.cp(l, @ldir) }
    end
    # copy system mpirun
    # FileUtils.cp(`which mpirun`, "#{ldir}/mpirun")
    `cp $(which mpirun) #{@ldir}/mpirun`
    puts "########## Isolated  ##########"
  end

  def run(opts={}, &blk)
    if not @isolate_called
      raise "Error: included Isolatable, but didn't call isolate()."
    end
    
    super(opts, &blk)
  end

  # redefine commmand to sbcast things over and call from isolated directory
  def command(c=nil)
    if c
      tdir = "/scratch/#{ENV['USER']}/igor/#{Process.pid}"
      @params[:tdir] = tdir
      ignore :tdir, :command
      c = %Q[
        if [[ ! -d "#{tdir}" ]]; then 
          srun mkdir -p #{tdir};
          ls #{tdir};
          echo $(hostname);
          for l in $(ls #{@ldir}); do
            echo $l; sbcast #{@ldir}/$l #{tdir}/${l};
          done;
        fi;
        #{c}
      ].tr("\n "," ")
    end
    return super(c)
  end
end

Igor do
  # machine-specific settings
  case `hostname`
  when /pal/
    params { machine "pal" }
    sbatch_flags << "--time=30:00" << "--account=pal" << "--partition=pal" << "--exclusive"
    $srun = "srun --cpu_bind=verbose,rank --label --kill-on-bad-exit"
    SHMMAX=34359738368
  when /n\d+/ # (sampa)
    params { machine "sampa" }
    sbatch_flags << "--partition=grappa" << "--exclusive"
    $srun = "srun --resv-ports --cpu_bind=verbose,rank --label --kill-on-bad-exit"
    SHMMAX=12884901888
  else
    params { machine `hostname` }
    $srun = "srun"
  end
  
  GFLAGS = Params.new {
    global_memory_use_hugepages 0
           num_starting_workers 64
                 loop_threshold 64
     aggregator_autoflush_ticks 1e5.to_i
           aggregator_max_flush 0
            periodic_poll_ticks 20000
                     chunk_size 100
                   load_balance 'steal'
                  flush_on_idle 0
                   poll_on_idle 1
          rdma_workers_per_core 2**4
                    target_size 2**12
          rdma_buffers_per_core 16
                 rdma_threshold 64
               shared_pool_size 2**16
                shared_pool_max 2**14
                     stack_size 2**19
             locale_shared_size SHMMAX
           global_heap_fraction 0.5
            flatten_completions 1
                 flat_combining 1
  }
  
  params { grappa_version 'asplos14' }
  
  class << GFLAGS
    def expand
      self.keys.map{|n| "--#{n}=%{#{n}}"}.join(' ')
    end
  end
  
  # parses JSON stats and colon-delimited fields
  parser {|cmdout|
    require 'json'

    h = {}

    # remove leading MPI/GLOG line prefixes
    cmdout.gsub!(/^\[[\d,]+\]\<\w+\>:/m){ '' } # remove pal header
    cmdout.gsub!(/^\d+:\s+/m){ '' }                 # remove sampa header
    cmdout.gsub!(/^I\d+ .*?\d+\] /m){ '' }          # remove glog header
    
    stats = []

    # scan, parse and filter JSON blocks
    cmdout.gsub!(/^STATS{.*?^}STATS/m) {|m|

      m.gsub!(/STATS/m){''} # remove tag
      m.gsub!(/\n/){''} # remove newlines
      m.gsub!(/:(\s+),/m) {": null,"}
      
      # get rid of double underscores, since sequel/sqlite3 don't like them
      m.gsub!(/__/m){ "_" }

      # get rid of pesky 'nan's if they show up
      m.gsub!(/: -?nan/, ': 0')


      blob = JSON.parse(m)
      flat = {}
      blob.to_enum(:flat_each).each {|k,v| flat[k.intern] = v }
      stats << flat
      ''
    }

    # transpose to dict of arrays rather than array of dicts
    combined = {}
    stats.each {|s| s.each_pair{|k,v| (combined[k] ||= []) << v }}

    # aggregate each and merge into h
    combined.each {|k,a|
      if a.all_numbers?
        # compute average of stats from multiple runs
        h[k] = a.reduce(:+).to_f/a.size
      else
        # concat into a single string (collapse to single value if all the same)
        h[k] = (a.uniq.size == 1) ? a.uniq[0].to_s : a.join(",")
      end
    }

    # parse out traditional fields (name: number)
    cmdout.gsub!(/^\s*(?<key>[\w_]+):\s+(?<value>#{REG_NUM})\s*$/){ m = $~
      h[m[:key].downcase.to_sym] = m[:value].to_f
      ''
    }
    
    # parse out string fields: name: 'value'
    cmdout.gsub!(/^\s*(?<key>[\w_]+):\s+'(?<value>.*)'\s*$/){ m = $~
      h[m[:key].downcase.to_sym] = m[:value]
      ''
    }
  
    if h.keys.length == 0 then
      puts "Error: didn't find any fields."
    end

    h # return hash
  }

end
