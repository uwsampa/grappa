#!/usr/bin/env ruby
require 'optparse'
require 'ostruct'

if ARGV.index('--')
  myargs = ARGV[0...ARGV.index('--')]
  remain = ARGV[ARGV.index('--')+1..ARGV.size]
else
  myargs = ARGV
  remain = []
end

DIR = File.expand_path(File.dirname(__FILE__))
puts "DIR = #{DIR}"

opt = OpenStruct.new
opt.nnode = 2
opt.ppn   = 1
opt.time  = '15:00'

OptionParser.new do |p|
  p.banner = "Usage: #{__FILE__} [options]"
  
  p.on('-n', '--nnode NODES', Integer, "Number of nodes to run the Grappa job with"){|n| opt.nnode = n }
  p.on('-p', '--ppn CORES', Integer, "Number of cores/processes per node"){|c| opt.ppn = c }
  p.on('-t', '--time TIME', 'Job time to pass to srun'){|t| opt.time = t }
  p.on('-e', '--test TEST', 'Run boost unit test program with given name (e.g. Aggregator_tests)'){|t| opt.test = t }
  
end.parse!(myargs)

srun_flags = %w[ --cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit ] \
          << "--task-prolog=#{DIR}/grappa_srun_prolog.sh" \
          << "--task-epilog=#{DIR}/grappa_srun_epilog.sh"

# "source" prolog
open("#{DIR}/grappa_srun_prolog.sh","r").each_line do |l|
  ENV[$~[1]] = $~[2] if l.match(/export\s+([\w_]+)=(.*)/)
end

case `hostname`
when /pal|node\d+/
  srun_flags << "--partition=pal,slurm" << "--account=pal"
  setarch = "setarch x86_64 -RL `pwd`/"
else
  srun_flags << "--partition=grappa" << "--resv-ports"
  setarch = ""
end

srun_flags << "--nodes=#{opt.nnode}" << "--ntasks-per-node=#{opt.ppn}" << "--time=#{opt.time}"

test = "#{opt.test}.test --log_level=test_suite --report_level=confirm --run_test=#{opt.test}" if opt.test

puts s = "srun #{srun_flags.join(' ')} -- #{test} #{setarch}#{remain.join(' ')}"
exec s
