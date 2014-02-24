#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/oopsla14.sqlite', :gups
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(%w[ gups.putget.exe gups.ext.exe ],
    File.dirname(__FILE__))
  
  GFLAGS.merge!({
    sizeA: 2**28.to_i,
    sizeB: 2**20.to_i,
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> cl { %Q[ %{tdir}/grappa_srun
    -- %{tdir}/%{exe}
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c['gups.ext.exe']
  
  sbatch_flags << "--time=10:00"
  
  params {
    exe 'gups.putget.exe', 'gups.ext.exe'
  }
  
  expect :gups_runtime
  
  interact # enter interactive mode
end
