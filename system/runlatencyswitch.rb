#!/usr/bin/env ruby
require '../experiment_utils'


db = "context_switch.db"
table = :context_switch_latency

cmd = "make mpi_test TARGET=ContextSwitchLatency_tests.test \
NNODE=%{nnode} \
PPN=%{ppn} \
VERBOSE_TESTS=1 \
SRUN_FLAGS=--time=5 \
GARGS=' \
--lines=%{touched_cachelines}' 2>&1 |tee out.txt"


params = {
    trial: [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20],
    nnode: [1],
    ppn: [1],
    #total_iterations: [51200000],
    machine: ['cluster'],
    touched_cachelines: [1,4,16,32,128,512,1024,8192,16000,50000,150000,500000,1000000,1500000,4000000],
    problem: ['switch_latency'],
}


parser = lambda{ |cmdout|
    records = {}

    # parse experiment specific results
    dict = /time = (?<switch_time>\d+\.\d+e-\d+)/.match(cmdout).dictionize
    
    if dict.empty? then
        raise "Output string does not match"
    end

    records.merge!(dict)

    records
}

run_experiments(cmd, params, db, table, &parser)
