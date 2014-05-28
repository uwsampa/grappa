#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :cc
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['../graphlab/cc.exe'])
  
  GFLAGS.merge!({
    path: '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4',
    format: 'bintsv4',
    max_iterations: 1024
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/cc.exe --metrics --vmodule graphlab=1
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=30:00"
  
  params {
    nnode       16
    ppn         32
    loop_threshold 1024
    num_starting_workers 512
    aggregator_autoflush_ticks 3e6.to_i
    periodic_poll_ticks 2e5.to_i
    global_heap_fraction 0.2
    shared_pool_chunk_size 2**15
  }
  
  expect :total_time
  @cols << :total_time
  @order = :total_time
  
  interact # enter interactive mode
end
