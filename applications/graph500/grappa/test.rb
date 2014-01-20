#!/usr/bin/env ruby

########################################################################
## This file is part of Grappa, a system for scaling irregular
## applications on commodity clusters. 

## Copyright (C) 2010-2014 University of Washington and Battelle
## Memorial Institute. University of Washington authorizes use of this
## Grappa software.

## Grappa is free software: you can redistribute it and/or modify it
## under the terms of the Affero General Public License as published
## by Affero, Inc., either version 1 of the License, or (at your
## option) any later version.

## Grappa is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## Affero General Public License for more details.

## You should have received a copy of the Affero General Public
## License along with this program. If not, you may obtain one from
## http://www.affero.org/oagpl.html.
########################################################################
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
