#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../../util/igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

$datasets="/sampa/home/bdmyers/graph_datasets"

Igor do
  include Isolatable
  
  database 'cqs.db', :queries

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['squares_partition.exe'],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 128
     aggregator_autoflush_ticks 100000
            periodic_poll_ticks 20000
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                   scale 20
                   edgefactor 16
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun.rb --nnode=%{nnode} --ppn=%{ppn}
    -- %{tdir}/squares_partition.exe
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       2
    ppn         2
    tag         'v1'
    machine 'grappa'
    query   'squares'
    algorithm 'partition-1-step'
  }
  
  run {
    trial 1,2,3
    nnode 4
    ppn 4
    scale 14,15
    edgefactor 4, 16
  } 

  run {
    trial 1,2,3
    nnode 12
    ppn 7
    scale 14,15
    edgefactor 4, 16
  }

  # required measures
  expect :query_runtime
 
  $filtered = results{|t| t.select(:nnode, :ppn, :scale, :run_at, :query_runtime, :ir4_final_count_max, :ir4_final_count_min, :results_count, :edges_transfered/:total_edges) }
    
  interact # enter interactive mode
end
