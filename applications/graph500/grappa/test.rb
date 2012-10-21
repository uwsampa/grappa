#!/usr/bin/env ruby
require "./bfs.rb"

$cmd = %Q[ make -j mpi_run TARGET=graph.exe NNODE=%{nnode} PPN=%{ppn} BFS=%{bfs}
  SRUN_FLAGS='--time=30:00'
  GARGS='
    --aggregator_autoflush_ticks=%{flushticks}
    --periodic_poll_ticks=%{pollticks}
    --num_starting_workers=%{nworkers}
    --async_par_for_threshold=%{threshold}
    --io_blocks_per_node=%{io_blocks_per_node}
    --io_blocksize_mb=%{io_blocksize_mb}
    --v=1
    -- -s %{scale} -e %{edgefactor} -p -%{generator} -f %{nbfs}
  '
].gsub(/[\n\r\ ]+/," ")

$params = {
  bfs: "bfs_local",
  scale: [16],
  edgefactor: [16],
  generator: "K",
  nbfs: [4],
  nnode: [8],
  ppn: [4],
  nworkers: [2048],
  flushticks: [4000000],
  pollticks: [80000],
  chunksize: [64],
  threshold: [128],
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
