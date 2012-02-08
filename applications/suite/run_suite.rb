#!/usr/bin/env ruby
EXP_HOME = ENV["EXP_HOME"]

require "#{EXP_HOME}/util.rb"

# parse: string -> hash
# Extracts data from program output and returns a hash of values for the new row
parse = Proc.new do |cmdout|
  s = ""
  cmdout.each_line {|l| s += l }
  
  pattern = /computeGraph[\w\s\(\)]+(\d+\.\d+) \s sec\.$
            .*connected \s components\: \s (\d+)$.*
            connectedComponents[\w\s\(\)]+(\d+\.\d+) \s sec\.$
            .*
            matches: \s (\d+)$
            .*pathIsomorphism[\w\s()]+(\d+\.\d+) \s sec.$
            .*
            triangles: \s (\d+)
            .*triangles[\w\s()]+(\d+\.\d+) \s sec.$
            .*Betweenness \s Centrality[A-Za-z\s:()=]+(\d+\.\d+)
            .*(\d+\.\d+) \s sec./mx
  data = {}
  s.match(pattern) {|m|
    data[:compute_graph_time] = m[0]
    data[:connected_components] = m[1]
    data[:connected_components_time] = m[2]
    data[:path_matches] = m[3]
    data[:path_isomorphism_time] = m[4]
    data[:triangles] = m[5]
    data[:triangles_time] = m[6]
    data[:centrality] = m[7]
    data[:centrality_time] = m[8]
  }
  puts data
  return data
end

$testing = true

['graphb'].each { |exe|
[1, 2, 4, 8].each { |max_procs|
[4, 5, 6, 7].each { |scale|
  data = runExperiment("mtarun -m #{max_procs} #{exe} #{scale}", parse, :suite, false)
  puts "inserted: #{data}"
}}}
