#!/usr/bin/env ruby
require "experiments"
require "../experiment_utils"

$db = "#{ENV['HOME']}/rdma-gups-sampa.db"
$table = :GUPS

#Compile command: echo "module load openmpi-x86_64 && make -j -C grappa/system TARGET=Gups_tests.test GARGS=' 
#--iterations $(( 1 << 30 )) 
#--load_balance=none 
#--num_starting_workers=1536 
#--aggregator_autoflush_ticks=20000000 
#--periodic_poll_ticks=200000 
#--global_memory_use_hugepages=false 
#--repeats=3 
#--async_par_for_threshold=1 
#--rdma=false 
#--target_size=$(( 1 << 16 )) 
#--sizeA=$(( 1 << 13 )) 
#--validate=false 
#--outstanding=$(( 1 << 18 ))' PPN=11 NNODE=12  mpi_test" | ssh grappa.sampa

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[ make -j mpi_test TARGET=Gups_tests.test NNODE=%{nnode} PPN=%{ppn}
    SRUN_RESERVE='--time=5:00'
    GARGS='
      --v=0
      --iterations=%{iterations}
      --repeats=%{repeats}
      --sizeA=%{sizeA}
      --rdma=%{rdma}
      --validate=%{validate}
      --outstanding=%{outstanding}
      --target_size=%{target_size}
      --load_balance=%{load_balance}
      --num_starting_workers=%{nworkers}
      --async_par_for_threshold=%{async_par_for_threshold}
      --aggregator_autoflush_ticks=%{flushticks}
      --periodic_poll_ticks=%{pollticks}
      --global_memory_use_hugepages=%{hugepages}
      --stack_offset=%{stack_offset}
      --flush_on_idle=%{flush_on_idle}
    '
  ].gsub(/[\n\r\ ]+/," ")
$machinename = "sampa"
$machinename = "pal" if `hostname` =~ /pal/

# map of parameters; key is the name used in command substitution
$params = {
#  experiment_id: [Time.now.to_i],
  experiment_id: [1361354591],

  iterations: [2**30],
  repeats: [1],



  nnode: [12,11,10,9,8,7,6,5,4,3,2],
#  nnode: [12],
  #ppn: [1],


  nworkers: [1536],

 trial: [1,2,3,4,5,6,7,8,9],
#  trial: [1,2,3],
#  trial: [4,5,6],
#  trial: [7,8,9],

  sizeA: [2**13,2**16],

  ppn: [12,11,10,9,8,7,6,5,4,3,2,1],
#  ppn: [10],
#  ppn: [6,8],

  target_size: [2**12,2**16],

  rdma: ["false", "true"],
#  rdma: ["false"],
#  rdma: ["true"],

  validate: ["false"],
  outstanding: [2**18],

  async_par_for_threshold: [1],

  stack_offset: [64],

  flushticks: [20000000],
  pollticks: [20000],

  flush_on_idle: ["true"],
#  target_tasks: expr('nworkers'),
  load_balance: ["none"],
  hugepages: ["false"],
  nproc: expr('nnode*ppn'),
  machine: [$machinename],
}

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $db, $table, &$json_plus_fields_parser)
end
