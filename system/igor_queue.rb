#!/usr/bin/env ruby
require_relative 'igor_datastructs'

Igor do
  include Isolatable
  
  @dbtable = :queue
  
  isolate "GlobalVector_tests.test"
  
  GFLAGS.merge!({
    ntrials: [1],
    nelems: [1024],
    vector_size: [1024],
    fraction_push: [0.5],
  })
  @params.merge! GFLAGS
  command @test_cmd['GlobalVector_tests', '--queue_perf']
  
  params {
    tag 'vector_locks'
    scale 10
    nelems expr('2**scale')
    ntrials 1
  }
  
  interact
end
