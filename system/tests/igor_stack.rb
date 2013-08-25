#!/usr/bin/env ruby
require_relative 'igor_queue'

Igor do
  include Isolatable
  
  @dbtable = :stack
  
  isolate "GlobalVector_tests.test"
  
  command @test_cmd['GlobalVector_tests', '--stack_perf']
  
  params {
    version 'matching_better'
  }
  
  interact
end
