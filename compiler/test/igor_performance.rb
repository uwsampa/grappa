#!/usr/bin/env ruby
require 'igor'
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/grappa.sqlite', :compiler_gups
  
  isolate ['performance.exe']
  
  GFLAGS.merge!({
    size_a:     [2**30],
    size_b:     [2**20],
    iterations: [3]
  })
  GFLAGS.delete :flat_combining
  params.merge! GFLAGS
  
  command "%{tdir}/grappa_srun.rb -- %{tdir}/performance.exe #{GFLAGS.expand}"
  
  params { nnode 2; ppn 1 }
  
  interact
end
