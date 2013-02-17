require 'igor'

Igor do
  sbatch_flags "--time=30:00 #{
      (`hostname` =~ /pal/) \
        ? '--account=pal --partition=pal' \
        : '--partition=grappa'
      }"
      
  # parses JSON stats and colon-delimited fields
  parser {|cmdout|
    def clean_json(str)
      str.gsub!(/STATS/m) { '' } # remove tag
      str.gsub!(/\n/m) { '' } # remove newlines inside JSON blob in case things got split up by GLOG/MPI
      str.gsub!(/:\s+,/m) { ': null,' }
      return str
    end

    h = {}

    # remove leading MPI/GLOG line prefixes
    cmdout.gsub!(/^\[[\d,]+\]\<\w+\>:/m){ '' } # remove pal header
    cmdout.gsub!(/^\d+: /m){ '' }                 # remove sampa header
    cmdout.gsub!(/^I\d+ .*?\d+\] /m){ '' }          # remove glog header

    stats = []

    # scan, parse and filter JSON blocks
    cmdout.gsub!(/^STATS{.*?^}STATS/m) {|m|
      blob = JSON.parse(clean_json(m))
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
