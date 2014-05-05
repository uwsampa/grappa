#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../../util/igor_common.rb'

exe='KMeansMR.exe'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable
  
  database '/people/bdmyers/hardcode_results/datapar.db', :kmeans

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(["#{exe}"],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 128
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                  
                   normalize 'true'
                   converge_dist 1e-15
                   input 'data/train_features.txt.bind'
                   relations '/people/bdmyers/escience/datasets'
                   k 2
                   maxiters 0
                   combiner 'true'
                   centers_compared 0
     
  }
  params.merge!(GFLAGS) 
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} -t 60
    -- %{tdir}/#{exe} --aggregator_autoflush_ticks=%{aggregator_autoflush_ticks} --periodic_poll_ticks=%{periodic_poll_ticks} 
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       2
    trial 1
    ppn         2
    vtag         'v4-goodtix'
    machine 'grappa'
    periodic_poll_ticks  20000
    aggregator_autoflush_ticks 400000
  }
  
run {
    trial 1
    nnode 4,8,16,32,64
    periodic_poll_ticks expr('nnode*2500')
    aggregator_autoflush_ticks expr('12500*nnode')
    ppn 16
    k 1024
    vtag 'scaling'
    input 'seaflow/tokyo_all_files.csv.bin'
    converge_dist 1e6
    normalize 'false'
    maxiters 10
    combiner 'true'
    centers_compared 0
}

#run {
#    trial 1
#    nnode 64
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#    ppn 16
#    k 2
#    vtag 'v11-runtimes3'
#    input 'seaflow/tokyo_all_files.csv.bin'
#    converge_dist 1e6
#    normalize 'false'
#    maxiters 10
#    combiner 'true'
#    centers_compared 0
#}

#run {
#    trial 1,2,3,4,5,6
#    nnode 64
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#    ppn 16
#    k 10,10000
#    vtag 'runtimes'
#    input 'seaflow/tokyo_all_files.csv.bin'
#    converge_dist 1e6
#    normalize 'false'
#    maxiters 10
#    combiner 'true'
#    centers_compared 0
#}

#run {
#    trial 1,2,3,4,5,6
#    nnode 64,32,16,100
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#    ppn 16
#    k 10
#    input 'seaflow/tokyo_all_files.csv.bin'
#    converge_dist 1e6
#    normalize 'false'
#    maxiters 50
#}
#run {
#    periodic_poll_ticks expr('nnode*2500')
#    aggregator_autoflush_ticks expr('12500*nnode')
#    trial 1,2,3,4,5,6
#    nnode 4,8,16
#    ppn 16
#    k 10
#    input 'seaflow/thompson1_all_files.csv.bin'
#    converge_dist 1e5
#    normalize 'false'
#    maxiters 100
#}

  #expect :query_runtime
 
#  $basic = results{|t| t.select(:vtag, :run_at,:nnode, :ppn, :query, :query_runtime, :in_memory_runtime, :nt)}
#  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  #interact # enter interactive mode
end
