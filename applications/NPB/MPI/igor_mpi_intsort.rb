#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
require_relative '../../../util/igor_common.rb'

Igor do
  include Isolatable

  database '~/exp/sosp.db', :intsort

  # isolate(%w[simple replicated replicated_csc].map{|v| "graph500_mpi_#{v}"},
          # File.dirname(__FILE__))
  
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    tag     'mpi'
    problem 'D'
    nnode   64ƒ
    ppn     16
    nproc   expr('nnode*ppn')
  }
  
  exes = []
  params[:nnode].product(params[:ppn],params[:problem]) do |nnode,ppn,problem|
    f = "bin/is.#{problem}.#{nnode*ppn}"
    if not File.exists? f
      puts s="make is NPROCS=#{nnode*ppn} CLASS=#{problem}"
      puts `#{s}`.strip
    end
    exes << f
  end
  puts exes
  isolate(exes, File.dirname(__FILE__))
  
  command "#{$srun} %{tdir}/is.%{problem}.%{nproc}"
  
  expect :mops_total
    
  $filtered = results{|t| t.select(:id, :problem, :nnode, :ppn, :run_at, :run_time, :mops_total, :mops_per_process) }
  
  interact # enter interactive mode
end
