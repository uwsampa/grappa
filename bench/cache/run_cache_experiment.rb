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
  pattern = /.*\] ([\w_]+), num_chunks: (\w+), total_read_time_ns: (\w+), avg_acquire_time_ns: (\w+), avg_release_time_ns: (\w+)/
  data = { :num_nodes => 1 }
  data = params.clone().merge(data)
  
  results = []
  
  cmdout.each_line do |line|
    d = data.clone()
    m = line.match(pattern)
    if m then
      d[:total_read_time_ns] = m[3].to_i
      d[:avg_acquire_time_ns] = m[4].to_i
      d[:avg_release_time_ns] = m[5].to_i
      
      results << d
    else
      puts line
    end
  end
  
  return results
end

$testing = true

## small (2 kB)
# Nelems = [1<<(11-3)]
# Nchunks = [1, 1<<2, 1<<4, 1<<6, 1<<8]
## larger (1 MB)
Nelems = [1<<(11-3)]
Cache_elems = [1, 1<<2, 1<<4, 1<<6, 1<<8]

['./cache_experiment.exe'].each { |exe|
['incoherent_ro', 'incoherent_rw'].each { |experiment|
[2].each { |num_procs|
Nelems.each { |nelems|
Cache_elems.each { |cache_elems|
  params = { :experiment=>experiment, :num_procs=>num_procs, :data_size=>nelems*8, :cache_size=>cache_elems*8 }
  if !options[:force] and run_already?(params) then
    puts "#{params.inspect} -- skipping..."
  else
    # puts "#{pp params}"
    ENV["GLOG_logtostderr"] = 1.to_s
    ENV["OMPI_MCA_btl_sm_use_knem"] = 0.to_s
    data = runExperiment("mpirun -l -H localhost -np #{num_procs} -- \
      #{exe} --#{experiment} --nelems=#{nelems} --cache_elems=#{cache_elems}", $exp_table) do |cmdout|
      parse(cmdout, params)
    end
  end
}}}}}
