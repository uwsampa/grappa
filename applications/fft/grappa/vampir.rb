#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.
require "./fft_common.rb"
$params[:debug] = "VTRACE=1 VTRACE_SAMPLED=1 GOOGLE_PROFILER=1 CPUPROFILE_FREQUENCY=20 CPUPROFILE=gperf.fft.\$$SLURM_PROCID"
$params[:glog_args] = ""
$params[:fft_args] = ""

$params[:fft] = "fft"
$params[:scale] = [28]
$params[:nnode] = 12
$params[:ppn] = 2
$params[:tag] = "test"

parse_cmdline_options()
$opt_force = true

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
