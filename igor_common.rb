
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require 'igor'

# add some Grappa-specific things to Igor so they're available at the prompt
module Igor
  def sq
    case `hostname`
    when /pal/
      puts `squeue -ppal`
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
    shared_dir = File.dirname(__FILE__) unless shared_dir
    @ldir = "#{File.expand_path shared_dir}/.igor/#{Process.pid}"
    puts "making #{@ldir}"
    FileUtils.mkdir_p(@ldir)
    
    exes = [exes] unless exes.is_a? Array
    exes << 'mpirun' << "#{File.dirname(__FILE__)}/bin/grappa_srun.rb" \
                     << "#{File.dirname(__FILE__)}/bin/grappa_srun_prolog.sh" \
                     << "#{File.dirname(__FILE__)}/bin/grappa_srun_epilog.sh"
    
    exes.each do |exe|
      exe = `which #{exe}`.strip if not File.exists? exe
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
      ignore :tdir
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
  when /n\d+/ # (sampa)
    params { machine "sampa" }
    sbatch_flags << "--partition=grappa" << "--exclusive"
    $srun = "srun --resv-ports --cpu_bind=verbose,rank --label --kill-on-bad-exit"
  else
    params { machine `hostname` }
    $srun = "srun"
  end
  
  # parses JSON stats and colon-delimited fields
  parser {|cmdout|
    require 'json'

    h = {}

    # remove leading MPI/GLOG line prefixes
    cmdout.gsub!(/^\[[\d,]+\]\<\w+\>:/m){ '' } # remove pal header
    cmdout.gsub!(/^\d+:\s+/m){ '' }                 # remove sampa header
    cmdout.gsub!(/^I\d+ .*?\d+\] /m){ '' }          # remove glog header

    # get rid of double underscores, since sequel/sqlite3 don't like them
    cmdout.gsub!(/__/m){ "_" }

    stats = []

    # scan, parse and filter JSON blocks
    cmdout.gsub!(/^STATS{.*?^}STATS/m) {|m|

      m.gsub!(/STATS/m){''} # remove tag
      m.gsub!(/\n/){''} # remove newlines
      m.gsub!(/:(\s+),/m) {": null,"}

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
