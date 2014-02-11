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
  
  database '/sampa/home/bdmyers/hardcode_results/cqs.db', :queries

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['Query.exe'],
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
                   query 'SquareQuery'
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} -t 60
    -- %{tdir}/Query.exe
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60 --exclude=n72"
  
  params {
    nnode       2
    ppn         2
    tag         'v1'
    machine 'grappa'
    query   'SquareQuery','SquarePartition4way'
  }
  
#  run {
#    trial 1,2,3
#    nnode 4
#    ppn 4
#    edgefactor 16
#    scale 16,17
#    query 'SquarePartitionBushy4way' #'SquareQuery' #'SquarePartition4way','SquareBushyPlan'
#  } 
  
  run {
    trial 1
    nnode 4
    ppn 4
    edgefactor 16
    scale 18
    query 'SquarePartitionBushy4way', 'SquareBushyPlan' #'SquareQuery' #'SquarePartition4way','SquareBushyPlan'
  } 

#  run {
#    trial 1
#    nnode 9
#    ppn 9
#    scale 12
#    edgefactor 16
#  }
#
  # required measures
  expect :query_runtime
 
  $basic = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query, :query_runtime, :results_count)}
  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  interact # enter interactive mode
end
