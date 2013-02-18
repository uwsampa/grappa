#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do  
  database '~/exp/sosp.db', :bfs
  sbatch_flags "#{@sbatch_flags} --time=2:00:00"
  
  command "srun graph500_mpi_%{mpibfs} %{scale} %{edgefactor}"
  
  params {
    mpibfs      'simple'
    nnode       2
    ppn         1
    scale       20
    edgefactor  16
  }
  
  params {
    experiment 'variants'
    mpibfs  'custom', 'simple', 'replicated', 'replicated_csc', 'one_sided'
  }
  
  interact # enter interactive mode
end
