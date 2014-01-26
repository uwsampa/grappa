#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../util/igor_common.rb'

Igor do
  database '~/exp/pgas.sqlite', :queue
  
  @params.merge! GFLAGS
  
  @sbatch_flags.delete_if{|e| e =~ /--time/} << "--time=1:00:00"
  
  @test_cmd = -> test, extras { %Q[ ../bin/grappa_srun --test=#{test} --no-verbose -- #{GFLAGS.expand} #{extras}] }
  command @test_cmd['GlobalVector_tests','']
    
  params {
    nnode 2
    ppn   1
  }
  
  interact # enter interactive mode
end
