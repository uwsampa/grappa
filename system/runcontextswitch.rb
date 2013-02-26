#!/usr/bin/env ruby
require '../experiment_utils'


db = "context_switch.db"
table = :context_switch

cmd = "make mpi_test TARGET=ContextSwitchRate_tests.test \
NNODE=%{nnode} \
PPN=%{ppn} \
VERBOSE_TESTS=1 \
GARGS=' \
--num_starting_workers=%{num_workers} \
--test_type=%{problem} \
--private_array_size=%{private_array_size} \
--iters_per_task=%{iters_per_task}'"


params = {
    trial: [1,2,3],
    nnode: [1],
    ppn: [1],
    #total_iterations: [51200000],
    num_workers: [12000, 40000, 128000],#[2,4,5,6,8,16,24,32,48,64,96,128,200,256,300, 512, 600,700,800,900,1024,1200,1500,1800,2048,4096,5000,6000,7000],
    iters_per_task: [7000],#expr('total_iterations/num_workers'),
    actual_total_iterations: expr('iters_per_task*num_workers'),
    machine: ['cluster'],
    problem: ['yields'],
    private_array_size: [0]#[1,16]#,64,256,512,8192]
}


parser = lambda{ |cmdout|
    records = {}

    # parse experiment specific results
    dict = /time = (?<total_time>\d+\.\d+), avg_switch_time = (?<avg_switch_time>\d+\.\d+e-\d+)/.match(cmdout).dictionize
    
    if dict.empty? then
        raise "Output string does not match"
    end

    records.merge!(dict)

    records
}

run_experiments(cmd, params, db, table, &parser)
