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
# opt.nnode = 2
# opt.ppn   = 1
# opt.time  = '2:00:00'
opt.freeze_on_error = false
opt.verbose = true

OptionParser.new do |p|
  p.banner = "Usage: #{__FILE__} [options]"

  p.on('-n', '--nnode NODES', Integer, "Number of nodes to run the Grappa job with"){|n| opt.nnode = n }
  p.on('-p', '--ppn CORES', Integer, "Number of cores/processes per node"){|c| opt.ppn = c }
  p.on('-t', '--time TIME', 'Job time to pass to srun'){|t| opt.time = t }
  p.on('-e', '--test TEST', 'Run boost unit test program with given name (e.g. Aggregator_tests)'){|t| opt.test = t }
  p.on('-f', '--[no-]freeze-on-error', "Freeze all the jobs when there's an error"){|f| opt.freeze_on_error = f }
  p.on('-v', '--[no-]verbose', "Verbose tests"){|v| opt.verbose = v }

end.parse!(myargs)

srun_flags = %w[ --cpu_bind=verbose,rank --label --kill-on-bad-exit ] \
          << "--task-prolog=#{DIR}/grappa_srun_prolog.rb" \
          << "--task-epilog=#{DIR}/grappa_srun_epilog.sh"

# "source" prolog
require_relative "grappa_srun_prolog.rb"

setarch = ""

case `hostname`
when /pal|node\d+/
  srun_flags << "--partition=pal" << "--account=pal"
  # disable address randomization (doesn't seem to actually fix pprof multi-node problems)
  # setarch = "setarch x86_64 -RL "
else
  srun_flags << "--partition=grappa" << "--resv-ports"
end

srun_flags << "--nodes=#{opt.nnode}" if opt.nnode
srun_flags << "--ntasks-per-node=#{opt.ppn}" if opt.ppn
srun_flags << "--time=#{opt.time}" if opt.time
ENV["GASNET_FREEZE_ON_ERROR"] = opt.freeze_on_error ? "1" : "0"

if opt.verbose
  verbose_test = '--log_level=test_suite --report_level=confirm'
else
  verbose_test = '--log_level=nothing --report_level=no'
end
test = "#{opt.test}.test #{verbose_test} --run_test=#{opt.test}" if opt.test
# jacob's preferred test options
#test = "#{opt.test}.test --log_level=nothing --report_level=no --run_test=#{opt.test}" if opt.test

s = "srun #{srun_flags.join(' ')} -- #{test} #{setarch}#{remain.join(' ')}"
puts s
$stdout.flush
exec s
