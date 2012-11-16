#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.
require "./fft_common.rb"
$params[:debug] = "DEBUG=1"
$params[:verify] = "--verify"

$params[:fft] = "fft"
$params[:scale] = 3
$params[:nnode] = 2
$params[:ppn] = 2
$params[:tag] = "test"

parse_cmdline_options()
$opt_force = true

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
