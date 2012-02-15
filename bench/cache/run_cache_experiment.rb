#!/usr/bin/env ruby
EXP_HOME = ENV["EXP_HOME"]
require "optparse"
require "sequel"
require "pp"
require "#{EXP_HOME}/util.rb"


loadConfigs()
options = commonOptions()
$exp_table = :cache

# parse: string -> hash or [hash,...]
# Extracts data from program output and returns a hash of values for the new row
def parse(cmdout, params)
  # puts cmdout
  pattern = /.*\] num_chunks: (\d+), total_read_time_ns: (\d+)/
  data = { :num_nodes => 1, :cache_size => (2**8)*8 }
  data = params.clone().merge(data)
  
  results = []
  
  cmdout.each_line do |line|
    d = data.clone()
    m = line.match(pattern)
    if m then
      d[:num_chunks] = m[1].to_i
      d[:total_read_time_ns] = m[2].to_i
      
      results << d
    end
  end
  
  return results
end

$testing = true

['./local_caching.exe'].each { |exe|
[2].each { |num_procs|
  params = {:num_procs=>num_procs}
  if !options[:force] and run_already?(params) then
    puts "#{params.inspect} -- skipping..."
  else
    # puts "#{pp params}"
    ENV["GLOG_logtostderr"] = 1.to_s
    ENV["OMPI_MCA_btl_sm_use_knem"] = 0.to_s
    data = runExperiment("mpirun -l -H localhost -np #{num_procs} -- #{exe}", $exp_table) do |cmdout|
      parse(cmdout, params)
    end
  end
}}
