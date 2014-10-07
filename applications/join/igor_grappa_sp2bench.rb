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
  
  database "#{ENV['HOME']}/hardcode_results/cqs.db", :sp2bench

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
  }
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} -t 60
    -- %{tdir}/#{queryexe} --vmodule=grappa_Q*=%{emitlogging}
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    trial 1
    nnode       2
    ppn         2
    vtag         'v10-materializing'
    machine "#{machine}"
    query "#{query}"
    plan "#{plan}"
    hash_local_cells 16*1024
    emitlogging 0
  }
  params.merge!(GFLAGS) 
  
#  run {
#    trial 1,2,3
#    nnode 4
#    ppn 4
#    edgefactor 16
#    scale 16,17
#    query 'SquarePartitionBushy4way' #'SquareQuery' #'SquarePartition4way','SquareBushyPlan'
#  } 


#  run {
#    nnode 1
#    ppn 1
#  }
#
#  
#  run {
#    nnode 4
#    ppn 6
#  }
#  run {
#    nnode 6
#    ppn 6
#  }
#  run {
#    nnode 10
#    ppn 6
#  }
#run {
#    trial 1,2,3#1,2,3,4,5,6
#    nnode 4,8,16#4,8,16#1,4,32,64#1,4,6,10,32,64
#    ppn 6
#    emitlogging 0
#}


#run  {
#    vtag 'scalability'
#    trial 1,2,3
#    nnode 4,8,16,32,64
#    ppn 16
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#}
#run  {
#    vtag 'symmetric-hash'
#    trial 1,2,3
#    nnode 4,8,16,32,64
#    ppn 16
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#}
#run  {
#    vtag 'shuffle-compare'
#    trial 1,2,3
#    nnode 16
#    ppn 16
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#}
#run {
#    trial 1
#    nnode 16
#    ppn 6,16
#    loop_threshold 512
#}
#run {
#    trial 1
#    nnode 16
#    ppn 16
#    aggregator_autoflush_ticks 200000,400000
#    periodic_poll_ticks expr('aggregator_autoflush_ticks/5')
#}
run  {
    vtag 'sym'
    trial 1,2,3
    nnode 16
    ppn 16
    periodic_poll_ticks expr('nnode*2500')
    aggregator_autoflush_ticks expr('12500*nnode')
}


  # required measures
  #expect :query_runtime
  #expect :shit
 
  $basic = results{|t| t.select(:vtag, :run_at,:nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :nt)}
#  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  #interact # enter interactive mode
end
