#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../util/igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable
  
  database 'sosp.db', :uts

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['uts_grappa.exe'],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
    global_memory_use_hugepages 1
           num_starting_workers 1024,2048
                 loop_threshold 16
     aggregator_autoflush_ticks 50000,100000
            periodic_poll_ticks 20000
                     chunk_size 100
                   load_balance 'steal'
                  flush_on_idle 0
                   poll_on_idle 1
                    vmodule "uts_grappa*=2"
                     v       1
                     flat_combine false
                     flat_combine_threshold expr('num_starting_workers/2')
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun
    -- %{tdir}/uts_grappa.exe
    #{expand_flags(*GFLAGS.keys)} --vertices_size=%{vertices_size}
    -- %{tree_args} 
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=60"
  
  params {
    nnode       8, 16, 32 
    ppn         1,2,4,8,12
    tag         'none'
    tree        'T1','T1L','T2L','T3L'
    tree_args   expr('ENV[tree]')
    problem     'uts-mem'
    vertices_size expr("ENV['SIZE'+tree]")

  }
  
  expect :generate_runtime, :search_runtime
 
  $filtered = results{|t| t.select(:id, :nnode, :ppn, :tree, :run_at, :search_runtime) }
    
  interact # enter interactive mode
end
