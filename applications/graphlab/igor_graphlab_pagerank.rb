#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :pagerank
  
  params { version 'grappa' }
  
  isolate 'pagerank.exe'
  
  GFLAGS.merge!({
    path: '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4',
    format: 'bintsv4',
    max_iterations: 1024,
    trials: 3,
    scale: 10
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/pagerank.exe --metrics --vmodule graphlab=1
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
    shared_pool_chunk_size 2**15
    global_heap_fraction 0.2
    # max_iterations 10
  }
  
  expect :total_time
  
  @cols << :aggregator_autoflush_ticks  
  @cols << :total_time_mean
  @order = :total_time_mean
  
  # scaling
  p = {
     16 => {scale:28},
     32 => {scale:29},
     64 => {scale:30},
    128 => {scale:31},
  }
  
  interact # enter interactive mode
end
