#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do  
  database '~/exp/sosp-test.db', :bfs  
  
  command "srun graph500_mpi_simple %{scale} %{edgefactor}"
  
  params {
    nnode       2
    ppn         1
    scale       16
    edgefactor  16
  }
  
  interact # enter interactive mode
end
