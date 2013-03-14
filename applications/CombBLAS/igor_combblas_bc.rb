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
  
  params {
    nnode       2
    ppn         2
    scale       16
    edgefactor  16
    k4approx    6
    nperbatch   4
  }
  
  $filtered = results{|t| t.select(:id, :scale, :nnode, :ppn, :run_at, :k4approx, :centrality_teps) }
  
  interact
end
