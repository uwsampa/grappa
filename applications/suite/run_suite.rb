#!/usr/bin/env ruby
EXP_HOME = ENV["EXP_HOME"]

require "#{EXP_HOME}/util.rb"
loadConfigs()

# parse: string -> hash
# Extracts data from program output and returns a hash of values for the new row
parse = Proc.new do |cmdout|
  # puts cmdout
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
  m = cmdout.match(pattern)
  if m then
    data[:compute_graph_time] = m[1].to_f
    data[:connected_components] = m[2].to_i
    data[:connected_components_time] = m[3].to_f
    data[:path_matches] = m[4].to_i
    data[:path_isomorphism_time] = m[5].to_f
    data[:triangles] = m[6].to_i
    data[:triangles_time] = m[7].to_f
    data[:centrality] = m[8].to_f
    data[:centrality_time] = m[9].to_f
  end
  # p data
  data
end

$testing = true

['./graphb'].each { |exe|
[1, 2, 4, 8].each { |max_procs|
[4, 5, 6, 7].each { |scale|
  data = runExperiment("#{exe} #{scale}", parse, :suite)
  p data
}}}
