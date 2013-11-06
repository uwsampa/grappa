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
  
  database 'join.db', :twohop

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['twohop.exe'],
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
  
  command %Q[ %{tdir}/grappa_srun.rb
    -- %{tdir}/twohop.exe
    #{expand_flags(*GFLAGS.keys)}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       6,8,12
    ppn         6
    tag         'none'
    with_hash   true
    with_distinct 1
  }
  
  """
  run {
    fin $datasets+'/gnutella/p2p-Gnutella04.txt.und'
    file_num_tuples 79988
    trial 1,2 
  } 
  
  run {
    fin $datasets+'/gnutella/p2p-Gnutella08.txt.und'
    file_num_tuples 41554
    trial 1,2
  }

  run {
      fin $datasets+'/twitter/twitter_rv_sample.net'
      file_num_tuples 214954
      trial 1,2
  }
  run {
      fin $datasets+'/twitter/twitter4ME.htm.tab'
      file_num_tuples 4460398
      trial 1,2,3
  }
"""
  
  expect :twohop_runtime
 
  #$filtered = results{|t| t.select(:id, :nnode, :ppn, :tree, :run_at, :search_runtime) }
    
  interact # enter interactive mode
end
