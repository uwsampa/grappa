#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do
  include Isolatable

  database '~/exp/sosp-test.db', :bfs

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate("graph500_mpi_simple", File.dirname(__FILE__))
  
  command "srun graph500_mpi_simple %{scale} %{edgefactor}"
  
  params {
    nnode       2
    ppn         1
    scale       16
    edgefactor  16
  }
  
  run
  
  interact # enter interactive mode
end
