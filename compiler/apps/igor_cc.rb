#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

Igor do
  include Isolatable
  
  database(table: :cc)
  
  exes = %w[ cc.putget.exe
           cc.ext.exe
           cc.hand.exe
           cc.ext.nolocalize.exe
         ]
  
  params { exe *exes }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    scale: 16,
    concurrent_roots: 8
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/%{exe} --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  expect :ncomponents, :components_time
  
  params {
    nnode 12; ppn 8; scale 23
    num_starting_workers 768
    loop_threshold 256
    concurrent_roots 512
  }
  
  interact # enter interactive mode
end
