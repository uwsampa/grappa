#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :cc
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate ['graph_new.exe']
  
  GFLAGS.merge!({
    bench: ["cc"],
    cc_hash_size: [1024],
    cc_concurrent_roots: [64],
    cc_insert_async: [0]
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun.rb
    -- %{tdir}/graph_new.exe
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    nnode       2
    ppn         1
    scale       26
    edgefactor  16
    
    loop_threshold 64
    num_starting_workers 512
    aggregator_autoflush_ticks 1e5.to_i
    periodic_poll_ticks 2e4.to_i
    stack_size 2**16    
    
  }
  
  expect :ncomponents
  
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
    
  interact # enter interactive mode
end
