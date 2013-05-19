#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../igor_common.rb'

Igor do
  database '~/exp/pgas.sqlite', :queue

  # isolate everything needed for the executable so we can sbcast them for local execution
  params.merge!(GFLAGS)
  
  @test_cmd = -> test, extras { %Q[ ../bin/grappa_srun.rb --no-verbose --test=#{test} -- #{GFLAGS.expand} #{extras}] }
  command @test_cmd['GlobalVector_tests','']
  
  sbatch_flags.delete_if{|e| e =~ /--time/} << "--time=1:00:00"
  
  params {
    nnode 2
    ppn   1
    log_nelems 10
    nelems expr('2**scale')
  }
  
  interact # enter interactive mode
end
