#!/usr/bin/env ruby
require "experiments"
trap("SIGINT") { puts ""; exit! } # exit (somewhat more) gracefully
parse_cmdline_options() {|opts|
  $opt_dry = false
  opts.on('-d','--dry', "Dry test run of just one experiment.") {
    $opt_dry = true
    $opt_noinsert = true
    $opt_force = true
  }
}

db = ENV["HOME"]+"/exp/softxmt.db"
table = :cache_mp

cmd = "mpirun -l -rcfile ./.mpirunrc.cache_experiment -H n01,n04 -np %{num_procs} -- \
      %{exe} --%{experiment} --nelems=%{nelems} --cache_elems=%{cache_elems} --num_threads=%{num_threads} --logtostderr"
params = {
  exe:          './cache_experiment.exe',
  experiment:   'incoherent_all_remote',
  num_threads:  [16, 32, 64, 128, 256, 384, 512],
  num_procs:    [2, 4, 8, 24, 48],
  num_nodes:    [2],
  nelems:       [1<<14, 1<<16, 1<<18, 1<<20, 1<<22, 1<<24],
  cache_elems:  [1<<1, 1<<2, 1<<4, 1<<6, 1<<7, 1<<8]
}
if $opt_dry then
  params = params.merge({num_threads:[32], nelems:[1<<16], cache_elems:[1<<6], num_procs:[4]})
end

parser = lambda {|cmdout|
  # pattern = /.* total_read_ticks: (?<total_read_ticks>#{REG_NUM}), avg_acquire_ticks: (?<avg_acquire_ticks>#{REG_NUM}), avg_release_ticks: (?<avg_release_ticks>#{REG_NUM})/
  pattern = /{.*}/
  
  results = []
  
  cmdout.each_line do |line|
    if m = pattern.match(line) then
      results << eval(m[0]) rescue puts "eval error!\n#{line}"
    else
      begin
        puts line[/\w+:\w+/] + line[/\].*/]
      rescue NoMethodError, TypeError
        puts line
      end
    end
  end
  
  results
}

# ENV["GLOG_logtostderr"] = "1"
ENV["OMPI_MCA_btl_sm_use_knem"] = "0"

run_experiments(cmd, params, db, table, &parser)
