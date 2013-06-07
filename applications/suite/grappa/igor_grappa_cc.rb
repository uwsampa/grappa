#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/pgas.sqlite', :cc
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate 'suite.exe'
  
  GFLAGS.merge!({
    cc_hash_size: [1024],
    cc_concurrent_roots: [64],
    cc_insert_async: [0]
  })
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun.rb --nnode=%{nnode} --ppn=%{ppn} --time=4:00:00
    -- %{tdir}/suite.exe
    #{GFLAGS.expand}
    -- --scale=%{scale} --ckpt --components
  ].gsub(/\s+/,' ')
  
  sbatch_flags.delete_if{|e| e =~ /--time/} << "--time=4:00:00"
  
  params {
    version 'private_task'
    
    nnode    16
    ppn      16
    scale    16
    
    loop_threshold 64
    num_starting_workers 512
    aggregator_autoflush_ticks 1e5.to_i
    periodic_poll_ticks 2e4.to_i
    stack_size 2**16    
  }
  
  expect :ncomponents
  
  interact # enter interactive mode
end
