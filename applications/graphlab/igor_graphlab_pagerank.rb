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
    max_iterations: 1024
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
    ppn   16
    num_starting_workers 512
    aggregator_autoflush_ticks 3e6.to_i
    periodic_poll_ticks 2e5.to_i
    loop_threshold 1024
    trial 1
    max_iterations 10
  }
  
  expect :total_time
  
  @cols << :total_time
  @order = :total_time
  
  interact # enter interactive mode
end
