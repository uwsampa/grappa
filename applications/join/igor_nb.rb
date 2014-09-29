#!/usr/bin/env ruby
require 'igor'

query = ARGV[0]
plan = ""
if ARGV.length == 2 then
    plan = "#{ARGV[1]}_"
end

queryexe = "grappa_#{plan}#{query}.exe"


machine = ENV['GRAPPA_CLUSTER']
if not machine then
    raise "need to set GRAPPA_CLUSTER to pal or sampa"
end


# inherit parser, sbatch_flags
load '../../../../util/igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

$datasets="/sampa/home/bdmyers/graph_datasets"

Igor do
  include Isolatable
  
  database "#{ENV['HOME']}/hardcode_results/nb.db", :msd

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(["#{queryexe}"],
    File.dirname(__FILE__))#,
   # symlinks=["sp2bench_1m"])
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 128
     aggregator_autoflush_ticks 100000
            periodic_poll_ticks 20000
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                   nt ENV['NTUPLES'].to_i
                   input_file_testdata '/sampa/home/bdmyers/escience/datasets/millionsong/YearPredictionMSD_test_8attr.txt'
                   input_file_conditionals '/sampa/home/grappa-cmake/build/Make+Release/applications/join/conditionals'
  }
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} -t 60
    -- %{tdir}/#{queryexe} --vmodule=grappa_*=%{emitlogging}
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    trial 1
    nnode       2
    ppn         2
    vtag         'v1'
    machine "#{machine}"
    query "#{query}"
    plan "#{plan}"
    hash_local_cells 16*1024
    emitlogging 0
  }
  params.merge!(GFLAGS) 

  run  {
    trial 1,2,3
    nnode 16
    ppn 16
    periodic_poll_ticks 16*2500
    aggregator_autoflush_ticks 16*12500
}
  

  # required measures
  #expect :query_runtime
  #expect :shit
 
  $basic = results{|t| t.select(:vtag, :run_at,:nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :nt)}
#  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  #interact # enter interactive mode
end
