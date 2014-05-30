#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :pagerank
  
  params { version 'grappa-getput' }
  
  isolate 'gups-getput.exe'
  
  GFLAGS.merge!({
      sizeB: 2**32
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/gups-getput.exe
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    nnode 16
    ppn   32
    num_starting_workers 1024, 1536, 2048
    aggregator_autoflush_ticks 1e6.to_i, 2e6.to_i, 3e6.to_i, 4e6.to_i, 5e6.to_i, 6e6.to_i, 7e6.to_i, 10e6.to_i
    periodic_poll_ticks 1e6.to_i
    loop_threshold 16
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
