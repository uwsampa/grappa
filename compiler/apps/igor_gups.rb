#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/oopsla14.sqlite', :gups

  exes = %w[ gups.putget.exe
           gups.ext.exe
           gups.ext.noasync.exe
           gups.void.exe
           gups.hand.exe
           gups.blocking.hand.exe
         ]
  
  params { exe *exes }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    log_array_size: 28,
    log_iterations: 20,
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
