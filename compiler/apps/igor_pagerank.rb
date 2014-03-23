#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

Igor do
  include Isolatable
  
  database(table: :pagerank)
  
  exes = %w[ pagerank.putget.exe
           pagerank.ext.exe
           pagerank.hand.exe
           pagerank.ext.nohop.exe
         ]
  
  params { exe *exes }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    scale: 23,
    global_heap_fraction: 0.2,
    shared_pool_memory_fraction: 1.0
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/%{exe} --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  expect :pagerank_time
  
  params {
    scale 23; nnode 12; ppn 8
    num_starting_workers 1024
    loop_threshold 128
  }
  
  interact # enter interactive mode
end
