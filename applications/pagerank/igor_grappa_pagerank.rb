#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../util/igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :pagerank

  exec_name = 'pagerank.exe'

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(["#{exec_name}"],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
    global_memory_use_hugepages 1
           num_starting_workers 1024,2048
                 loop_threshold 16
     aggregator_autoflush_ticks 50e3.to_i,100e3.to_i
            periodic_poll_ticks 20000
                     chunk_size 100
                   load_balance 'steal'
                  flush_on_idle 0
                   poll_on_idle 1
                   loop_threshold 16

                   # spM configuration
                   nnz_factor 10
                   scale      16  

                   # accepted damping parameter for pagerank
                   damping 0.8

                   # reasonable error
                   epsilon 0.001
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun
    -- %{tdir}/#{exec_name}
    #{expand_flags(*GFLAGS.keys)} 
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       8
    ppn         8
    tag         'none'
    problem     'pagerank'
  }
  
  expect :iterations_time, :pagerank_time
 
  $filtered = results{|t| t.select(:id, :nnode, :ppn, :scale, :run_at, :iterations_time_mean, :iterations_time_count) }
    
  interact # enter interactive mode
end
