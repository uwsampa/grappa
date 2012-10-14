#!/usr/bin/env ruby
require "./sort.rb"

$params = {
  scale: [16],
  log2buckets: [7],
  log2maxkey: [10],
  nnode: [12],
  ppn: [2],
  nworkers: [1024],
  flushticks: [2000000],
  pollticks: [20000],
  chunksize: [64],
  threshold: [64],
  io_blocks_per_node: [1],
  io_blocksize_mb: [512],
  nproc: expr('nnode*ppn'),
  machine: [$machinename],
}
$opt_force = true

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
