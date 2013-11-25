#!/usr/bin/env ruby
require 'igor'
require_relative '../../util/igor_common.rb'

Igor do
  include Isolatable
  
  database '~/exp/grappa.sqlite', :compiler_gups
  
  isolate ['performance.exe']
  
  command "%{tdir}/grappa_srun.rb -- %{tdir}/performance.exe #{GFLAGS.expand}"
  
  params {
    sizeA      2**30
    sizeB      2**20
    iterations 3
  }  
  
  interact
end
