#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../../igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :intsort

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['intsort.exe'], File.dirname(__FILE__))
  
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
  
  command %Q[ %{tdir}/grappa_srun.rb --nnode=%{nnode} --ppn=%{ppn} --time=4:00:00
    -- %{tdir}/intsort.exe
    #{expand_flags(*GFLAGS.keys)}
    -- --class=%{problem} %{verify}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=1:00:00"
  
  params {
    nnode   2
    ppn     1
    problem 'A'
    verify  ''
    tag     'grappa'
  }
  
  expect :mops_total
  
  $sel = ->t{ t.select(:id, :problem, :nnode, :ppn, :run_at, :run_time, :mops_total, :mops_per_process) }
  
  interact # enter interactive mode
end
