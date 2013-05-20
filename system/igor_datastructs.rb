#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../igor_common.rb'

Igor do
  database '~/exp/pgas.sqlite', :queue
  
  @test_cmd = -> test, extras { %Q[ ../bin/grappa_srun.rb --no-verbose --test=#{test} -- #{GFLAGS.expand} #{extras}] }
  command @test_cmd['GlobalVector_tests','']
  
  sbatch_flags.delete_if{|e| e =~ /--time/} << "--time=1:00:00"
  
  params {
    nnode 2
    ppn   1
  }
  
  interact # enter interactive mode
end
