#!/usr/bin/env ruby
require_relative 'igor_hashset'

Igor do
  @dbtable = :hashmap
  
  command @test_cmd['GlobalHash_tests', '--map_perf']
  
  params {
    log_nelems 10; nelems expr('2**log_nelems')
    log_max_key 10; max_key expr('2**log_max_key')
    log_global_hash_size 10; global_hash_size expr('2**log_global_hash_size')
    ntrials 1
  }
  
  interact
end
