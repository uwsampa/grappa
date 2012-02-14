#!/usr/bin/env ruby
EXP_HOME = ENV["EXP_HOME"]
require "optparse"
require "sequel"
require "#{EXP_HOME}/util.rb"

loadConfigs()
$exp_table = :cache

# parse: string -> hash or [hash,...]
# Extracts data from program output and returns a hash of values for the new row
def parse(cmdout, params)
  # puts cmdout
  pattern = /num_chunks: (\d+), total_read_time_ns: (\d+)/
  data = { :num_nodes => 1, :cache_size => (2**8)*8 }
  data = params.clone().merge(data)
  
  results = []
  
  cmdout.each_line do |line|
    d = data.clone()
    m = line.match(pattern)
    if m then
      d[:num_chunks] = m[1].to_i
      d[:cache_size] = m[2].to_i
    end
    results << d
  end
  
  return results
end

$testing = true

['./local_caching.exe'].each { |exe|
[1].each { |num_procs|
  params = {:max_procs=>max_procs, :scale=>scale}
  if run_already?(params) then
    puts "#{params} -- skipping..."
  else
    # puts "#{pp params}"
    data = runExperiment("mpirun -np #{num_procs} #{exe} #{scale}", $exp_table) do |cmdout|
      parse(cmdout, params)
    end
    puts "#{data}"
  end
}}
