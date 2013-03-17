#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../../../igor_common.rb'

Igor do
  include Isolatable

  database '~/exp/sosp.db', :intsort

  # isolate(%w[simple replicated replicated_csc].map{|v| "graph500_mpi_#{v}"},
          # File.dirname(__FILE__))
  
  
  sbatch_flags << "--time=4:00:00"
  
  params {
    problem 'D'
    nnode   8,16,32,64,128
    ppn     2,4,8,16
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
    
  $filtered = results{|t| t.select(:id, :mpibfs, :scale, :nnode, :ppn, :run_at, :run_time, :mops_total) }
  
  interact # enter interactive mode
end
