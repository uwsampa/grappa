#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative_to_symlink '../../../util/igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable
  
  database '~/exp/gen.db', :bfs

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
                              v 1
                        vmodule ''
  }
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} --time=4:00:00
    -- %{tdir}/graph.exe
    #{expand_flags(*GFLAGS.keys)}
    -- -s %{scale} -e %{edgefactor} -f %{nbfs} -w
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=6:00:00"
  
    params {
    nnode       2
    ppn         2
    scale       16
    edgefactor  16
    nbfs        0
    tag         'grappa'
  }
  
  expect :max_teps
  
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
  
  interact # enter interactive mode
end
