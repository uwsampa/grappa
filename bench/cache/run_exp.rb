#!/usr/bin/env ruby
require "experiments"
trap("SIGINT") { puts ""; exit! } # exit (somewhat more) gracefully

db = ENV["HOME"]+"/exp/softxmt.db"
table = :cache_mp

cmd = "mpirun -l -H n01,n04 -np %{num_procs} -- \
      %{exe} --%{experiment} --nelems=%{nelems} --cache_elems=%{cache_elems} --num_threads=%{num_threads} --logtostderr"
params = {
  exe:          './cache_experiment.exe',
  experiment:   'incoherent_all',
  num_threads:  [8, 32, 64],
  num_procs:    [2],
  num_nodes:    [2],
  nelems:       [1<<8, 1<<10, 1<<12],
  cache_elems:  [1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7, 1<<8]
}

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
