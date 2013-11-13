#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :bfs
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['graph_new.exe'],
    File.dirname(__FILE__))
  
  GFLAGS.merge!({
    beamer_alpha: 20,
    beamer_beta: 20,
    scale: 16,
    edgefactor: 16,
    nbfs: 8,
    shared_pool_max: 64,
    cas_flatten: 0,
    shared_pool_size: 2**20,
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = ->{ %Q[ %{tdir}/grappa_srun.rb
    -- %{tdir}/graph_new.exe
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    nnode       2
    ppn         1
    scale       27
    edgefactor  16
    nbfs        8
  }
  
  expect :max_teps
  
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
    
  interact # enter interactive mode
end
