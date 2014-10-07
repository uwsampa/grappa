#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :sssp
  
  params { version 'grappa' }
  
  isolate 'sssp.exe'
  
  GFLAGS.merge!({
    path: '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4',
    format: 'bintsv4',
    max_iterations: 1024,
    trials: 3
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/sssp.exe --metrics --vmodule graphlab=1 --max_degree_source
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    nnode 16
    ppn   32
    num_starting_workers 512
    aggregator_autoflush_ticks 3e6.to_i
    periodic_poll_ticks 2e5.to_i
    loop_threshold 1024
    max_iterations 1024
    global_heap_fraction 0.15
    shared_pool_memory_fraction 0.3
  }
  
  expect :total_time
  
  @cols << :total_time_mean
  @order = :total_time_mean
  
  interact # enter interactive mode
end
