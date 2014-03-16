#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

Igor do
  include Isolatable
  
  database(table: :bfs)
  
  exes = %w{
    bfs.putget.exe
    bfs.ext.exe
    bfs.ext.noasync.exe
    bfs.hand.exe
  }
  
  isolate exes
  
  GFLAGS.merge!({
    v: 0,
    scale: 16,
    nbfs: 8,
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
  
  @c = -> { %Q[ %{tdir}/grappa_srun --no-freeze-on-error
    -- %{tdir}/%{exe} --metrics
    #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params { exe *exes }
  
  expect :bfs_mteps
  
  interact # enter interactive mode
end
