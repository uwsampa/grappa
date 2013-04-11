#!/usr/bin/env ruby
require '../experiment_utils'


db = "context_switch.db"
table = :context_switch

cmd = "make mpi_run TARGET=ContextSwitchRate_bench.exe \
NNODE=%{nnode} \
PPN=%{ppn} \
SRUN_FLAGS= \
GARGS=' \
--num_starting_workers=%{num_workers} \
--num_test_workers=%{num_test_workers} \
--test_type=%{problem} \
--private_array_size=%{private_array_size} \
--global_memory_use_hugepages=%{global_memory_use_hugepages} \
--periodic_poll_ticks=%{periodic_poll_ticks} \
--load_balance=%{load_balance} \
--stack_size=%{stack_size} \
--stats_blob_enable=%{stats_blob_enable} \
--stats_blob_ticks=%{stats_blob_ticks} \
--readyq_prefetch_distance=%{readyq_prefetch_distance} \
--iters_per_task=%{iters_per_task}' 2>&1 |tee out.txt"


params = {
    trial: [2],
    nnode: [1],
    total_iterations: [100e6.to_i+1], # about 5 seconds
    readyq_prefetch_distance: [4],
    num_total_workers: [8,16,32,64,4096,8196,16000],#[1500,2048,2400,256,65000,35000,20000,24000,30000,40000,45000,50000,55000,128,60000,70000,80000,200000,100000,1000,4096,12000,16000,8196,400000,512],#[1<<5, 1<<7, 18000, 65536,100000, 400000, 500000, 550000],# 1<<5, (1<<5)+(1<<4), 1<<6, 1<<7, 1<<8, (1<<8)+(1<<7), 1<<9, 1<<10, (1<<10)+(1<<9), 1<<11, (1<<11)+(1<<10), 1<<12, (1<<12)+(1<<11), 1<<13, (1<<14)+(1<<13), 1<<15,(1<<15)+(1<<14), 1<<16, (1<<16)+(1<<15), 1<<17, 1<<18],# 48, 56, 64, 128, 256, 512, 600, 650, 700, 750, 800, 850, 900, 950, 1100,1200,1300,1400,1600,1800,2100,2300, 2800, 3100, 3600, 4000],#, 4500, 5000, 5500, 6000, 6500, 15000, 18000, 20000, 24000, 26000, 30000, 40000,60000],
    ppn: [2,4,6],#[1,8],
    #num_workers: [4,8,1024,24000,64000,96000,128000,240000,8192],#[16,32,64,128,256,1024,2048,4096,8000],#expr('num_total_workers/ppn'),
    #num_workers: [4,6,10,18,34],
    num_test_workers: expr('num_total_workers/ppn'),
    num_workers: expr('num_test_workers+2'), #[1024,1500,1750,2000,2250,2500,2750,3000,3250,3500, 3750, 4250, 4500, 5500],#[2,4,5,6,8,16,24,32,48,64,96,128,200,256,300, 512, 600,700,800,900,1024,1200,1500,1800,2048,4096,5000,6000,7000],
    stack_size: [1<<14],#expr('1<<([19,Math.log2(((1<<19) * (12000.to_f / num_test_workers)))].min)'),
    iters_per_task: expr('total_iterations/num_test_workers'),
    actual_total_iterations: expr('iters_per_task*num_test_workers'),
    machine: ['cluster'],
    problem: ['yields'],
    problem_flavor: ['new-worker-multi-queue-prefetch2x-fixdq-pincpualt'],
    timing_style: ['each-core-no-local-barrier'],
    private_array_size: [0],#[1,16]#,64,256,512,8192]
    periodic_poll_ticks: [1e8.to_i],
    global_memory_use_hugepages: [true],
    guard_stack: [false],
    stats_blob_enable: [false],
    stats_blob_ticks: [6e9.to_i],
    stack_prefetch_amount: [3],
    worker_prefetch_amount: [1],
    #prefetch_worker_struct: [1],
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

#run_experiments(cmd, params, db, table, &parser)
run_experiments(cmd, params, db, table, &$json_plus_fields_parser)
