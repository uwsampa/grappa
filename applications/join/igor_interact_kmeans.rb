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
  
  database '/sampa/home/bdmyers/hardcode_results/datapar.db', :kmeans

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(["#{exe}"],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 128
     aggregator_autoflush_ticks 100000
            periodic_poll_ticks 20000
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                  
                   normalize 'true'
                   converge_dist 1e-15
                   input 'data/train_features.txt.bind'
                   relations '/sampa/home/bdmyers/escience/datasets'
                   k 2
  }
  params.merge!(GFLAGS) 
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} -t 60
    -- %{tdir}/#{exe}
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    trial 1
    nnode       2
    ppn         2
    vtag         'v2-CSE'
    machine 'grappa'
  }
  
  # required measures
  #expect :query_runtime
  #expect :shit
 
  $basic = results{|t| t.select(:vtag, :run_at,:nnode, :trial, :ppn, :k, :iterations_runtime_count, :iterations_runtime_mean, :kmeans_runtime)}
#  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  interact # enter interactive mode
end
