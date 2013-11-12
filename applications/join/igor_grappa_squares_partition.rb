#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

$datasets="/sampa/home/bdmyers/graph_datasets"

Igor do
  include Isolatable
  
  database 'cqs.db', :triangles

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['squares_partition.exe'],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 128
     aggregator_autoflush_ticks 100000
            periodic_poll_ticks 20000
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                   scale 20
                   edgefactor 16
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun.rb
    -- %{tdir}/squares_partition
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       2
    ppn         2
    tag         'v1'
  }
  
  run {
    nnode 4
    ppn 4
    scale 14 
  } 

  run {
    nnode 12
    ppn 7
    scale 16
  }

  # required measures
  expect :squares_runtime
 
  $filtered = results{|t| t.select(:id, :nnode, :ppn, :scale, :run_at, :squares_runtime) }
    
  interact # enter interactive mode
end
