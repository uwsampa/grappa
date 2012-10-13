require "experiments"
require "json"
require "enumerator"

# monkeypatching
class Hash
  # recursively flatten nested Hashes
  def flat_each(prefix="", &blk)
    each do |k,v|
      if v.is_a?(Hash)
        v.flat_each("#{prefix}#{k}_", &blk)
      else
        yield "#{prefix}#{k}".to_sym, v
      end
    end
  end
end

def clean_json(str)
  str.gsub!(/\n/m) { '' } # remove newlines inside JSON blob in case things got split up by GLOG/MPI

  str.scan(/:.*?,/m) {|m| puts m}
  return str.gsub(/:(?<empty>\s+),/m) do m = $~
    case
    when m[:empty] then ": null,"
    end
  end
end

# parses JSON stats and colon-delimited fields
$json_plus_fields_parser = lambda {|cmdout|
  h = {}

  # remove leading MPI/GLOG line prefixes
  cmdout.gsub!(/^\d+: (.*?\] )?/m){ '' }
  
  # scan, parse and filter JSON blocks
  cmdout.gsub!(/^{.*?^}/m) {|m|
    blob = JSON.parse(clean_json(m))
    blob.to_enum(:flat_each).each {|k,v| h[k.intern] = v }
    ''
  }

  # parse out traditional fields (name: number)
  cmdout.gsub!(/^(?<key>[\w_]+): (?<value>#{REG_NUM})\s*$/){ m = $~
    h[m[:key].downcase.to_sym] = m[:value].to_f
    ''
  }
  
  if h.keys.length == 0 then
    puts "Error: didn't find any fields."
  end

  h # return hash
}

