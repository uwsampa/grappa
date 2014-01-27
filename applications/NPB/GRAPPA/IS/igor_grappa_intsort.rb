#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :intsort

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['intsort.exe'], File.dirname(__FILE__))
  
  GFLAGS.merge! Params.new {
    global_memory_use_hugepages 0
           num_starting_workers 64
                 loop_threshold 64
     aggregator_autoflush_ticks 1e6.to_i
            periodic_poll_ticks 20000
                     chunk_size 100
                   load_balance 'steal'
                  flush_on_idle 0
                   poll_on_idle 1
                              v 1
                        vmodule ''
  }
  GFLAGS.delete :flat_combining
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun --nnode=%{nnode} --ppn=%{ppn} --time=4:00:00
    -- %{tdir}/intsort.exe
    #{GFLAGS.expand}
    -- --class=%{problem} --niterations=%{niterations} %{verify}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=1:00:00"
  
  params {
    nnode   2
    ppn     1
    problem 'E'
    verify  ''
    version 'grappa'
    niterations 8
    
    aggregator_autoflush_ticks 1e6.to_i
    stack_size 2**17
    loop_threshold 512
    num_starting_workers 128
    shared_pool_max 2048
    shared_pool_size 2**16
  }
  
  expect :mops_total
  
  $sel = ->t{ t.select(:id, :problem, :nnode, :ppn, :run_at, :run_time, :mops_total, :mops_per_process) }
  
  interact # enter interactive mode
end
