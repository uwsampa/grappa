#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :pagerank
  
  params { version 'grappa-verbs' }
  
  isolate 'gups-verbs.exe'
  
  GFLAGS.merge!({
      sizeB: 2**30
  })
  GFLAGS.delete :flat_combining
  GFLAGS.delete :loop_threshold
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/gups-verbs.exe
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    nnode 16, 32, 64, 96, 128
    ppn   1
    num_starting_workers 512
    aggregator_autoflush_ticks 25e6.to_i
    periodic_poll_ticks 2e5.to_i
    loop_threshold 1024
    shared_pool_chunk_size 2**15
    global_heap_fraction 0.2
    # max_iterations 10

     sizeA 2**30
     mode "gups"
     batch_size 512
     dest_batch_size 8
  }
  
  expect :gups_throughput
  
  @cols << :aggregator_autoflush_ticks  
  @cols << :gups_throughput
  @order = :gups_throughput
  
  interact # enter interactive mode
end
