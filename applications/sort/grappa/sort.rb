#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require "experiments"
require "../../../experiment_utils"

$dbpath = File.expand_path("~/exp/grappa-sort.db")
$table = :intsort

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[ make mpi_run TARGET=sort.exe NNODE=%{nnode} PPN=%{ppn}
  SRUN_FLAGS='--time=10:00'
  GARGS='
    --aggregator_autoflush_ticks=%{flushticks}
    --periodic_poll_ticks=%{pollticks}
    --num_starting_workers=%{nworkers}
    --async_par_for_threshold=%{threshold}
    --v=1
    -- -s %{scale} -b %{log2buckets}
  '
].gsub(/[\n\r\ ]+/," ")
machinename = "sampa"
machinename = "pal" if `hostname` =~ /pal/

# map of parameters; key is the name used in command substitution
$params = {
  scale: [16],
  log2buckets: [6],
  nnode: [2],
  ppn: [4],
  nworkers: [4096],
  flushticks: [2000000],
  pollticks: [20000],
  chunksize: [64],
  threshold: [64],
  nproc: expr('nnode*ppn'),
  machine: [machinename],
}

run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
