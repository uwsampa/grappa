#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../igor_common.rb'

Igor do
  include Isolatable

  database '~/exp/sosp.db', :centrality

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate "Applications/betwcent"
  
  command "#{$srun} %{tdir}/betwcent RMAT:%{scale} %{k4approx} %{nperbatch}"
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    nnode       2
    ppn         2
    scale       16
    edgefactor  16
    k4approx    8
    nperbatch   128
  }
  
  $squares = [Params.new {nnode 16;  ppn 1,4,16},
              Params.new {nnode 32;  ppn 2,8},
              Params.new {nnode 64;  ppn 1,4,16},
              Params.new {nnode 128; ppn 2,8}]
  # $squares.each{|s| run{ merge!(s) } }
  
  expect :centrality_teps
  
  $filtered = results{|t| t.select(:id, :scale, :nnode, :ppn, :run_at, :k4approx, :centrality_teps) }
  
  interact
end
