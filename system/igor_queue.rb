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
    version 'vector_two_level'
    log_nelems 10
    nelems expr('2**log_nelems')
    vector_size expr('(2**log_nelems)*2')
    ntrials 1
  }
  
  interact
end
