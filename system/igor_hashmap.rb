#!/usr/bin/env ruby
require_relative 'igor_hashset'

Igor do
  @dbtable = :hashmap
  
  command @test_cmd['GlobalHash_tests', '--map_perf']
  
  interact
end
