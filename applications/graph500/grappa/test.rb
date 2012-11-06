#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.
require "./bfs.rb"

$cmd = %Q[ make -j mpi_run TARGET=graph.exe NNODE=%{nnode} PPN=%{ppn} BFS=%{bfs} DEBUG=1
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
    --global_memory_use_hugepages=%{global_memory_use_hugepages}
    --v=1 --vmodule bfs_beamer=2
    -- -s %{scale} -e %{edgefactor} -p -%{generator} -f %{nbfs}
  '
].gsub(/[\n\r\ ]+/," ")

$params[         :bfs] = "bfs_beamer"
$params[:beamer_alpha] = 20.0
$params[ :beamer_beta] = 20.0
$params[       :scale] = 16
$params[        :nbfs] = 4
$params[       :nnode] = 4
$params[         :ppn] = 4
$params[   :threshold] = 8
$params[         :tag] = "test"

parse_cmdline_options()
$opt_force = true

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
