#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative_to_symlink '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/oopsla14.sqlite', :bfs
  
  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(%w[ bfs.putget.exe bfs.ext.exe bfs.hand.exe ],
    File.dirname(__FILE__))
  
  GFLAGS.merge!({
    v: 0,
    scale: 16
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/%{exe} --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    exe 'bfs.putget.exe', 'bfs.ext.exe', 'bfs.hand.exe'
  }
  
  expect :bfs_mteps
  
  interact # enter interactive mode
end
