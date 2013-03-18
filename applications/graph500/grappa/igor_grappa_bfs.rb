#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

def reload_gflags
  gflags = Params.new {
    global_memory_use_hugepages 0
    num_starting_workers 64
  }
  params.merge!(gflags)
  
  command %Q[ %{tdir}/grappa_srun.rb --nnode=%{nnode} --ppn=%{ppn}
    -- %{tdir}/graph.exe
    #{expand_flags(*gflags.keys)}
    -- -s %{scale} -e %{edgefactor} -f %{nbfs}
  ].gsub(/\s+/,' ')
end

Igor do
  include Isolatable

  database '~/exp/sosp.db', :bfs

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['graph.exe',
    '../../../bin/grappa_srun.rb',
    '../../../bin/grappa_srun_prolog.sh',
    '../../../bin/grappa_srun_epilog.sh'],
    File.dirname(__FILE__))
  
  gflags = Params.new {
    global_memory_use_hugepages 0
    num_starting_workers 64
  }
  params.merge!(gflags)
  
  command %Q[ %{tdir}/grappa_srun.rb --nnode=%{nnode} --ppn=%{ppn}
    -- %{tdir}/graph.exe
    #{expand_flags(*gflags.keys)}
    -- -s %{scale} -e %{edgefactor} -f %{nbfs}
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    bfs         'bfs'
    nnode       2
    ppn         1
    scale       16
    edgefactor  16
    nbfs        8
  }
  
  expect :max_teps
    
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :min_time, :max_teps) }
  
  interact # enter interactive mode
end
