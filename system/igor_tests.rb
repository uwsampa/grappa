#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../igor_common.rb'

Igor do
  database '~/exp/test.sqlite', :vector
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  params.merge!(GFLAGS)
  
  @test_cmd = -> test { %Q[ ../bin/grappa_srun.rb --no-verbose --test=#{test} -- #{GFLAGS.expand}] }
  command @test_cmd['GlobalVector_tests']
  
  sbatch_flags << "--time=15:00"
  
  params {
    nnode 2
    ppn   1
    scale 10
    nelems expr('2**scale')
  }
  
  interact # enter interactive mode
end
