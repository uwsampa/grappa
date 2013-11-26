#!/usr/bin/env ruby
require 'igor'
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/grappa.sqlite', :compiler_gups
  
  isolate ['performance.exe']
  
  GFLAGS.delete :flat_combining
  GFLAGS.merge!({
    size_a:     [2**30],
    size_b:     [2**20],
    iterations: [3]
  })
  command "%{tdir}/grappa_srun.rb -- %{tdir}/performance.exe #{GFLAGS.expand}"
  params.merge! GFLAGS
  
  interact
end
