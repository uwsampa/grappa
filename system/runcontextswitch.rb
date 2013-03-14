#!/usr/bin/env ruby
require '../experiment_utils'


db = "context_switch.db"
table = :context_switch

cmd = "make mpi_test TARGET=ContextSwitchRate_tests.test \
NNODE=%{nnode} \
PPN=%{ppn} \
VERBOSE_TESTS=1 \
SRUN_FLAGS=--time=5 \
GARGS=' \
--num_starting_workers=%{num_workers} \
--num_test_workers=%{num_test_workers} \
--test_type=%{problem} \
--private_array_size=%{private_array_size} \
--global_memory_use_hugepages=%{global_memory_use_hugepages} \
--periodic_poll_ticks=%{periodic_poll_ticks} \
--load_balance=%{load_balance} \
--iters_per_task=%{iters_per_task}' 2>&1 |tee out.txt"


params = {
    trial: [1,2,3],
    nnode: [1],
    ppn: [8],
    total_iterations: [62.5e6.to_i], # about 5 seconds
    num_total_workers: [64,64*8,8000,16000,24000],# 48, 56, 64, 128, 256, 512, 600, 650, 700, 750, 800, 850, 900, 950, 1100,1200,1300,1400,1600,1800,2100,2300, 2800, 3100, 3600, 4000],#, 4500, 5000, 5500, 6000, 6500, 15000, 18000, 20000, 24000, 26000, 30000, 40000,60000],
    num_workers: expr('num_total_workers/ppn'),
    #num_workers: [1024,1500,1750,2000,2250,2500,2750,3000,3250,3500, 3750, 4250, 4500, 5500],#[2,4,5,6,8,16,24,32,48,64,96,128,200,256,300, 512, 600,700,800,900,1024,1200,1500,1800,2048,4096,5000,6000,7000],
    num_test_workers: expr('num_workers-2'),
    iters_per_task: expr('total_iterations/num_workers'),
    actual_total_iterations: expr('iters_per_task*num_workers'),
    machine: ['cluster'],
    problem: ['cvwakes'],
    problem_flavor: ['new-worker-old-queue'],
    timing_style: ['each-core'],
    private_array_size: [0],#[1,16]#,64,256,512,8192]
    periodic_poll_ticks: [1e6.to_i],
    global_memory_use_hugepages: [true],
    load_balance: ['none']
}


parser = lambda{ |cmdout|
    records = {}

    # parse experiment specific results
    dict = /cores_time_avg = (?<cores_time_avg>\d+\.\d+), cores_time_max = (?<cores_time_max>\d+.\d+), cores_time_min = (?<cores_time_min>\d+.\d+)/.match(cmdout).dictionize
    
    if dict.empty? then
        raise "Output string does not match"
    end

    records.merge!(dict)

    records
}

run_experiments(cmd, params, db, table, &parser)
