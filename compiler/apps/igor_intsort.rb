#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

Igor do
  include Isolatable
  
  database(table: :intsort)
  
  exes = %w[ intsort.putget.exe
           intsort.ext.exe
           intsort.ext.noasync.exe
           intsort.hand.exe
         ]
  
  params { exe *exes }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    cls: 'A',
    iterations: 3
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/%{exe} --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  expect :rank_time_mean
  
  params {
    cls 'D'; nnode 12; ppn 8
    num_starting_workers 768; loop_threshold 1024
  }
  
  interact # enter interactive mode
end
