#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../util/igor_common.rb'

Igor do
  database '~/exp/test.sqlite', :vector
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  params.merge!(GFLAGS)
  
  $cmd = -> { %Q[ ../bin/grappa_srun --no-verbose --test=%{name} -- #{GFLAGS.expand}] }
  command $cmd[]
  
  sbatch_flags.delete_if{|e| e =~ /--time/} << "--time=15:00"
  
  params {
    name 'GlobalVector_tests'
    nnode 2
    ppn   1
    scale 10
    nelems expr('2**scale')
  }
  
  interact # enter interactive mode
end
