#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/grappa_results/sosp/context_switch-final.db', :context_switch

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['ContextSwitchRate_bench.exe'],
    File.dirname(__FILE__))
 
  command "%{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} --cpu_bind=verbose,map_cpu:1,3,5,7,9,11,2,4,6,8,10,12 \
-- %{tdir}/ContextSwitchRate_bench.exe \
GARGS=' \
--num_starting_workers=%{num_workers} \
--num_test_workers=%{num_test_workers} \
--test_type=%{problem} \
--private_array_size=%{private_array_size} \
--global_memory_use_hugepages=%{global_memory_use_hugepages} \
--periodic_poll_ticks=%{periodic_poll_ticks} \
--load_balance=%{load_balance} \
--stack_size=%{stack_size} \
--stats_blob_enable=%{stats_blob_enable} \
--stats_blob_ticks=%{stats_blob_ticks} \
--readyq_prefetch_distance=%{readyq_prefetch_distance} \
--iters_per_task=%{iters_per_task}'"
                     
  sbatch_flags << "--time=10"
  
  params {
      # basic
      trial       1,2,3
      nnode       1
      ppn         1,2,3,4,5,6
      
      # problem type
      problem 'yields'
      timing_style 'each-core-no-local-barrier'
      problem_flavor 'new-worker-multi-queue-prefetch2x-fixdq-pincpualt'

      # problem features
      total_iterations 100e6.to_i+1
      num_total_workers 8,16,32,64,4096,8196,16000,18000,20000,24000,30000,35000,40000,45000,50000,60000,80000,100000,150000,200000,300000,400000
      num_test_workers expr('num_total_workers/ppn')
      num_workers expr('num_test_workers+2')
      iters_per_task expr('total_iterations/num_test_workers')
      actual_total_iterations expr('iters_per_task*num_test_workers')
      
      # prefetching
      stack_prefetch_amount 3         # 3
      worker_prefetch_amount 1        # 1
      readyq_prefetch_distance 4      # 4
      
      # good stationary configs
      periodic_poll_ticks 1e8.to_i
      stack_size 1<<14
      private_array_size 0
      stats_blob_enable false
      stats_blob_ticks 6e9.to_i
      load_balance 'none'
      guard_stack false
      global_memory_use_hugepages true
  }
  
  expect :context_switch_test_runtime_avg, :context_switch_test_runtime_min, :context_switch_test_runtime_max
    
  interact # enter interactive mode
end
