
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require 'igor'

$GLOG_FLAGS = "GLOG_logtostderr=1 GLOG_v=1"

$GRAPPA_LIBPATH = "/sampa/home/bholt/grappa-beta/tools/built_deps/lib:/usr/lib64/openmpi/lib:/sampa/share/boost_1_51_0/lib"

$GASNET_FLAGS = "GASNET_BACKTRACE=1 GASNET_FREEZE_SIGNAL=SIGUSR1 GASNET_FREEZE_ON_ERROR=1 GASNET_FREEZE=0 GASNET_NETWORKDEPTH_PP=96 GASNET_NETWORKDEPTH_TOTAL=1024 GASNET_AMCREDITS_PP=48 GASNET_PHYSMEM_MAX=1024M"


Igor do
  params { machine "sampa" }
  
  sbatch_flags "--time=30:00 #{
      (`hostname` =~ /pal/) \
        ? '--account=pal --partition=pal' \
        : '--partition=grappa'
      }"
      
  # parses JSON stats and colon-delimited fields
  parser {|cmdout|
    require 'json'

    h = {}

    # remove leading MPI/GLOG line prefixes
    cmdout.gsub!(/^\[[\d,]+\]\<\w+\>:/m){ '' } # remove pal header
    cmdout.gsub!(/^\d+: /m){ '' }                 # remove sampa header
    cmdout.gsub!(/^I\d+ .*?\d+\] /m){ '' }          # remove glog header

    stats = []

    # scan, parse and filter JSON blocks
    cmdout.gsub!(/^STATS{.*?^}STATS/m) {|m|

      m.gsub!(/STATS/m){''} # remove tag
      m.gsub!('\n'){''} # remove newlines
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
    cmdout.gsub!(/^(?<key>[\w_]+):\s+(?<value>#{REG_NUM})\s*$/){ m = $~
      h[m[:key].downcase.to_sym] = m[:value].to_f
      ''
    }
  
    if h.keys.length == 0 then
      puts "Error: didn't find any fields."
    end

    h # return hash
  }

end
