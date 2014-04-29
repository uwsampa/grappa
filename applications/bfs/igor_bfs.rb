#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :bfs
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['bfs_bags.exe'])
  
  GFLAGS.merge!({
    path: '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4',
    format: 'bintsv4',
    nbfs: 3,
    beamer_alpha: 20.0,
    beamer_beta: 20.0,
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/bfs_bags.exe --metrics --max_degree_source --vmodule bfs_bags=1
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=30:00"
  
  params {
    grappa_version 'beamer'
    nnode       16
    ppn         32
    loop_threshold 1024
    num_starting_workers 512
    # aggregator_autoflush_ticks 3e6.to_i
    # periodic_poll_ticks 2e5.to_i
    shared_pool_chunk_size 2**15
    global_heap_fraction 0.25
  }
  
  expect :total_time
  @cols << :total_time_mean
  @order = :total_time_mean
  
  interact # enter interactive mode
end
