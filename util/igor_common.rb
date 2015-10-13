
########################################################################
## Copyright (c) 2010-2015, University of Washington and Battelle
## Memorial Institute.  All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##     * Redistributions of source code must retain the above
##       copyright notice, this list of conditions and the following
##       disclaimer.
##     * Redistributions in binary form must reproduce the above
##       copyright notice, this list of conditions and the following
##       disclaimer in the documentation and/or other materials
##       provided with the distribution.
##     * Neither the name of the University of Washington, Battelle
##       Memorial Institute, or the names of their contributors may be
##       used to endorse or promote products derived from this
##       software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
## FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
## UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
## OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
## BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
## LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
## USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
## DAMAGE.
########################################################################

require 'igor'

# add some Grappa-specific things to Igor so they're available at the prompt
module Igor
  def sq
    case `hostname`
    when /pal/
      puts `squeue -ppal -o '%.7i %.4P %.17j %.8u %.2t %.10M %.6D %R'`
    else
      puts `squeue -o '%.7i %.9P %.17j %.8u %.2t %.10M %.6D %R'`
    end
  end
end

module Isolatable
  extend self # (so all methods become class methods, like Igor module)
  
  def self.included(base) # do stuff when 'include Isolate' is called
    base.extend(self) # add all of this module's definitions to base module
  end
  
  def isolate(exes_in, shared_dir=nil)
    exes = exes_in.clone
    
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
         shared_pool_chunk_size 2**13
                     stack_size 2**19
             # locale_shared_size SHMMAX
           global_heap_fraction 0.5
    shared_pool_memory_fraction 0.25
            flatten_completions 1
                 flat_combining 1
       log2_concurrent_receives 7
  }
  
  params { grappa_version 'osdi14'; version 'grappa' }
  
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

      # get rid of trailing ",", which JSON hates
      m.gsub!(/,\s*}/, '}')


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
  
  @cols = [
    :id,
    :nnode,
    :loop_threshold,
    :num_starting_workers,
  ]
  
  @order = :id
  
  def selected
    c = @cols
    o = @order
    results{select *c}.order(o)
  end
  
end

$twitter = '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4'
$friendster = '/pic/projects/grappa/friendster/bintsv4/friendster.bintsv4'
