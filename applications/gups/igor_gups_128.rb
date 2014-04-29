#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :pagerank
  
  params { version 'grappa' }
  
  isolate 'gups.exe'
  
  GFLAGS.merge!({
      sizeB: 2**34
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/gups.exe
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    nnode 128
    ppn   16
    num_starting_workers 512
    aggregator_autoflush_ticks 40e6.to_i, 45e6.to_i, 50e6.to_i, 55e6.to_i, 60e6.to_i, 65e6.to_i, 70e6.to_i
    periodic_poll_ticks 2e5.to_i, 1e6.to_i
    loop_threshold 1024
    shared_pool_chunk_size 2**15
    global_heap_fraction 0.2
    # max_iterations 10
  }
  
  expect :gups_throughput
  
  @cols << :aggregator_autoflush_ticks  
  @cols << :gups_throughput
  @order = :gups_throughput
  
  interact # enter interactive mode
end
