#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

Igor do
  include Isolatable
  
  database(table: :gups)

  exes = %w[
    hops.putget.exe
    hops.ext.exe
    hops.ext.noasync.exe
    hops.ext.nohop.exe
    hops.hand.exe
  ]
  
  params { exe *exes }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    log_array_size: 30,
    log_iterations: 28,
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> cl { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/%{exe} --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c['gups.ext.exe']
  
  sbatch_flags << "--time=10:00"
  
  expect :gups_runtime
  
  params {
    version 'special'
    nnode 12; ppn 8
    num_starting_workers 512
    loop_threshold 768
    global_heap_fraction 0.2
    shared_pool_memory_fraction 0.9
  }
  
  interact # enter interactive mode
end
