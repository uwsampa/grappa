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
                   maxiters 0
                   combiner 'true'
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
 
  $basic = results{|t| t.select(:vtag, :run_at,:nnode, :converge_dist, :normalize, :k, :centers_compared, :combiner, :iterations_runtime_count, :iterations_runtime_mean, :kmeans_broadcast_time_mean, :mr_mapping_runtime_mean, :mr_combining_runtime_mean, :mr_reducing_runtime_mean, :mr_reallocation_runtime_mean, :delegate_async_ops*4*8/1000000 / :iterations_runtime_count)}
  $info = jobs{|t| t.select(:run_at, :nnode, :ppn, :input, :vtag, :error, :outfile)}
  $out = jobs{|t| t.select(:query, :error, :outfile)}
#  $detail = results{|t| t.select(:nnode, :ppn, :scale, :edgefactor, :query_runtime, :ir5_final_count_max/(:ir5_final_count_min+1), :ir6_count, (:ir2_count+:ir4_count+:ir6_count)/:query_runtime, :edges_transfered/:total_edges, :total_edges) }
    
  interact # enter interactive mode
end
