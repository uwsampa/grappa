#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :bfs

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['graph.exe'],
    File.dirname(__FILE__))
  
  GFLAGS = Params.new {
    global_memory_use_hugepages 0
           num_starting_workers 64
                 loop_threshold 16
     aggregator_autoflush_ticks 3e6.to_i
            periodic_poll_ticks 20000
                     chunk_size 100
                   load_balance 'steal'
                  flush_on_idle 0
                   poll_on_idle 1
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun.rb
    -- %{tdir}/graph.exe
    #{expand_flags(*GFLAGS.keys)}
    -- -s %{scale} -e %{edgefactor} -f %{nbfs}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    nnode       2
    ppn         1
    scale       16
    edgefactor  16
    nbfs        8
    tag         'none'
  }
  
  expect :max_teps
  
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
    
  interact # enter interactive mode
end
