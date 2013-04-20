#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/sosp.db', :centrality

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['suite.exe'],
    File.dirname(__FILE__))
  
  params.merge!(GFLAGS)
  
  command %Q[ %{tdir}/grappa_srun.rb --nnode=%{nnode} --ppn=%{ppn} --time=4:00:00
    -- %{tdir}/suite.exe
    #{GFLAGS.expand}
    -- --scale=%{scale} %{centrality_flag} --kcent=%{kcent} --ckpt
  ].gsub(/\s+/,' ')
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    nnode    2
    ppn      1
    scale    16
    tag      'grappa'
    k4approx 4
    kcent    expr('2 ** k4approx')
    # centrality 'single', 'multi'
    # centrality_flag expr('--centrality+{single:"", multi:"=multi"}[centrality.to_sym]')
    centrality_flag '--centrality', '--centrality=multi'
    
    aggregator_autoflush_ticks 1e6.to_i
    periodic_poll_ticks 2e5.to_i
    loop_threshold 64
    num_starting_workers 512
    
  }
  
  expect :centrality_teps
  
  interact # enter interactive mode
end
