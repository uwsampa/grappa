#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/osdi.sqlite', :cc
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['../graphlab/cc.exe'])
  
  GFLAGS.merge!({
    path: '/pic/projects/grappa/twitter/bintsv4/twitter-all.bintsv4',
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/cc.exe --vmodule graphlab=1
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=30:00"
  
  params {
    nnode       16
    ppn         16
    loop_threshold 512
  }
  
  expect :total_time
      
  interact # enter interactive mode
end
