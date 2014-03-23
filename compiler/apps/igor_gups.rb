#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

Igor do
  include Isolatable
  
  database(table: :gups)

  exes = %w[ gups.putget.exe
           gups.ext.exe
           gups.ext.noasync.exe
           gups.void.exe
           gups.hand.exe
           gups.blocking.hand.exe
           gups.hop.base.exe
           gups.hop.putget.exe
           gups.hop.hand.exe
           gups.hop.ext.exe
           gups.hop.ext.noasync.exe
           gups.hop.ext.nohop.exe
         ]
  
  params { exe *exes }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    log_array_size: 31,
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
  
  interact # enter interactive mode
end
