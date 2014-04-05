#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/osdi.sqlite', :pagerank
  
  params { version 'grappa' }
  
  isolate 'pagerank.exe'
  
  GFLAGS.merge!({
    path: '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4',
    format: 'bintsv4',
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/pagerank.exe --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    nnode 16
    ppn   16
    num_starting_workers 512
    loop_threshold 1024
    trial 1
    max_iterations 10
  }
  
  expect :total_time
  
  interact # enter interactive mode
end
