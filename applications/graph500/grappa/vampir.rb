#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require "experiments"
require "../../../experiment_utils"

$dbpath = File.expand_path("~/exp/grappa-bfs.db")
$table = :graph500_vampir

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[ VT_FILE_PREFIX='vtbfs' make -j mpi_run TARGET=graph.exe NNODE=%{nnode} PPN=%{ppn} BFS=%{bfs} VTRACE=1 VTRACE_SAMPLED=1 GOOGLE_PROFILER=1
  SRUN_FLAGS='--time=30:00'
  GARGS='
    --aggregator_autoflush_ticks=%{flushticks}
    --periodic_poll_ticks=%{pollticks}
    --num_starting_workers=%{nworkers}
    --async_par_for_threshold=%{threshold}
    --io_blocks_per_node=%{io_blocks_per_node}
    --io_blocksize_mb=%{io_blocksize_mb}
    --beamer_alpha=%{beamer_alpha}
    --beamer_beta=%{beamer_beta}
    --v=1
    -- -s %{scale} -e %{edgefactor} -p -%{generator} -f %{nbfs}
  ;
  mkdir -p prof/%{bfs}.%{scale}.%{name};
  mv vtbfs.* prof/%{bfs}.%{scale}.%{name};
  rm graph.exe.* 
  '
].gsub(/[\n\r\ ]+/," ")
$machinename = "sampa"
$machinename = "pal" if `hostname` =~ /pal/

# map of parameters; key is the name used in command substitution
$params = {
  bfs: "bfs_beamer",
  beamer_alpha: 20,
  beamer_beta: 20,
  name: "beamer",
  scale: [23],
  edgefactor: [16],
  generator: "K",
  nbfs: [2],
  nnode: [12],
  ppn: [4],
  nworkers: [1024],
  flushticks: [3000000],
  pollticks: [80000],
  chunksize: [64],
  threshold: [8],
  io_blocks_per_node: [1],
  io_blocksize_mb: [512],
  nproc: expr('nnode*ppn'),
  machine: [$machinename],
}
parse_cmdline_options()
$opt_force = true

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
