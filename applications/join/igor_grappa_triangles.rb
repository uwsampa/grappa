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
  
  database 'join.db', :triangles

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['triangles.exe'],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
           num_starting_workers 1024
                 loop_threshold 16
     aggregator_autoflush_ticks 100000
            periodic_poll_ticks 20000
                     chunk_size 100
                   load_balance 'none'
                  flush_on_idle 0
                   poll_on_idle 1
                   fin 'NULL'
                   file_num_tuples 0
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun
    -- %{tdir}/triangles.exe
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       2,4,6,8,12
    ppn         6
    tag         'none'
    with_hash   true
  }
  
  run {
    fin $datasets+'/gnutella/p2p-Gnutella04.txt.und'
    file_num_tuples 79988
    
  } 
  
  run {
    fin $datasets+'/gnutella/p2p-Gnutella08.txt.und'
    file_num_tuples 41554
  }

  run {
      fin $datasets+'/twitter/twitter_rv_sample.net'
      file_num_tuples 214954
      trial 2
  }
  
  expect :triangles_runtime
 
  #$filtered = results{|t| t.select(:id, :nnode, :ppn, :tree, :run_at, :search_runtime) }
    
  interact # enter interactive mode
end
