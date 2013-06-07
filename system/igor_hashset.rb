#!/usr/bin/env ruby
require_relative 'igor_datastructs'

Igor do
  @dbtable = :hashset
  
  include Isolatable
  isolate 'GlobalHash_tests.test'
  
  GFLAGS.merge!({
    nelems: [1024],
    ntrials: [1],
    max_key: [1024],
    global_hash_size: [1024],
    fraction_lookups: [0.5],
    insert_async: [0],
  })
  @params.merge!(GFLAGS)
  command @test_cmd['GlobalHash_tests', '--set_perf']
  
  params {
    version 'fc_looks_fixed'
    log_nelems 10; nelems expr('2**log_nelems')
    log_max_key 10; max_key expr('2**log_max_key')
    global_hash_size expr('2**log_max_key')
    ntrials 1
  }
  
  interact
end
