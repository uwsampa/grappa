#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require "experiments"
require "../../../experiment_utils"

$dbpath = File.expand_path("~/exp/grappa-fft.db")
$table = :fft

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[ make -j mpi_run TARGET=fft.exe NNODE=%{nnode} PPN=%{ppn} 
  SRUN_FLAGS='--time=10:00'
  GARGS='%{scale}'
].gsub(/[\n\r\ ]+/," ")
$machinename = "sampa"
$machinename = "pal" if `hostname` =~ /pal/
huge_pages = ($machinename == "pal") ? false : true

# map of parameters; key is the name used in command substitution
$params = {
  scale: [22, 24, 30],
  nnode: [4, 8, 12],
  ppn: [1, 2, 4, 8, 12],
  nproc: expr('nnode*ppn'),
  machine: [$machinename],
  fft: "mpi",
}

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
