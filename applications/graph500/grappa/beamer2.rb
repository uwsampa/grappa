#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.
require "./bfs.rb"

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[ make -j mpi_run TARGET=graph.exe NNODE=%{nnode} PPN=%{ppn} BFS=%{bfs}
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
    -- -s %{scale} -e %{edgefactor} -pn -%{generator} -f %{nbfs}
  '
].gsub(/[\n\r\ ]+/," ")

$params[:scale]        = 26
$params[:bfs] = "bfs_beamer"
$params[:beamer_alpha] = 30
$params[:beamer_beta]  = 20
$params[:nworkers]     = 1024
$params[:flushticks]   = 3000000
$params[:pollticks]    =  100000
$params[:chunksize]    = 64
$params[:threshold]    = 8

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
