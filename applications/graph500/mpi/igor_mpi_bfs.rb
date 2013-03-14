#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do
  include Isolatable

  database '~/exp/sosp.db', :bfs

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(%w[simple replicated replicated_csc].map{|v| "graph500_mpi_#{v}"},
          File.dirname(__FILE__))
  
  command "#{$srun} %{tdir}/graph500_mpi_%{mpibfs} %{scale} %{edgefactor}"
  
  params {
    mpibfs      'simple'
    nnode       2
    ppn         1
    scale       20
    edgefactor  16
  }
    
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
  
  interact # enter interactive mode
end
