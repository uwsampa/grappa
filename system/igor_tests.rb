#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../igor_common.rb'

Igor do
  database '~/exp/test.db', :test

  # isolate everything needed for the executable so we can sbcast them for local execution
  params.merge!(GFLAGS)
  
  @test_cmd = -> test { %Q[ ../bin/grappa_srun.rb --test=#{test} -- #{GFLAGS.expand}] }
  command @test_cmd['New_loop_tests']
  
  sbatch_flags << "--time=15:00"
  
  params {
    nnode 2
    ppn   1
  }
  
  expect :max_teps
    
  interact # enter interactive mode
end
