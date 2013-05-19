#!/usr/bin/env ruby
require_relative 'igor_datastructs'

Igor do
  @dbtable = :hashset
  
  GFLAGS.merge!({
    nelems: [1024],
    ntrials: [1],
    max_key: [1024],
    global_hash_size: [1024]
  })
  command @test_cmd['GlobalHashTable_tests.test', '--set_perf']
  
  params {
    log_nelems 10; nelems expr('2**log_nelems')
    log_max_key 10; max_key expr('2**log_max_key')
    log_global_hash_size 10; global_hash_size expr('2**log_global_hash_size')
    ntrials 1
  }
  
  interact
end
