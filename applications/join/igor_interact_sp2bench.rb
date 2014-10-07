#!/usr/bin/env ruby
require 'igor'

queryexe = "grappa_Q1.exe"

# inherit parser, sbatch_flags
load '../../../../util/igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

$datasets="/sampa/home/bdmyers/graph_datasets"

Igor do
  include Isolatable
  
  database "#{ENV['HOME']}/hardcode_results/cqs.db", :sp2bench

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(["#{queryexe}"],
    File.dirname(__FILE__))#,
    #symlinks=["sp2bench_1m"])
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 128
     aggregator_autoflush_ticks 100000
            periodic_poll_ticks 20000
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                   nt ENV['NTUPLES'].to_i
  }
  params.merge!(GFLAGS) 
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} -t 60
    -- %{tdir}/#{queryexe}
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=20"
  
  params {
    trial 1
    nnode       2
    ppn         2
    tag         'v2-CSE'
    machine 'grappa'
    query "#{query}"
  }
  
#  run {
#    trial 1,2,3
#    nnode 4
#    ppn 4
#    edgefactor 16
#    scale 16,17
#    query 'SquarePartitionBushy4way' #'SquareQuery' #'SquarePartition4way','SquareBushyPlan'
#  } 





  # required measures
  #expect :query_runtime
  #expect :shit
 
  $basic = results{|t| t.select(:vtag,:run_at,:nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :nt)}
  $times = results{|t| t.select(:vtag,:run_at,:nnode, :ppn, :query, :plan, :query_runtime, :in_memory_runtime, :scan_runtime, :init_runtime, :nt, :join_coarse_result_count, :emit_count)}
  $hash = results{|t| t.select(:vtag,:run_at,:nnode, :ppn, :query,:in_memory_runtime, :join_coarse_result_count, :emit_count, :hash_local_lookups, :hash_remote_lookups)}
  $times9 = results{|t| t.select(:vtag,:run_at,:nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :scan_runtime, :init_runtime, :nt, :join_coarse_result_count, :emit_count).where(:vtag=>'v9-fixStr')}
  $timesLoop = results{|t| t.select(:vtag,:loop_threshold, :nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :scan_runtime, :init_runtime, :nt, :join_coarse_result_count, :emit_count)}
  $timesTicks = results{|t| t.select(:vtag, :periodic_poll_ticks, :nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :scan_runtime, :init_runtime, :nt, :join_coarse_result_count, :emit_count)}
  $info = jobs{|t| t.select(:run_at, :nnode, :ppn, :query, :vtag, :error, :outfile)}
  $out = jobs{|t| t.select(:query, :error, :outfile)}
#  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  interact # enter interactive mode
end
