#!/usr/bin/env ruby
require '../../experiment_utils'


db = "uts-compare.db"
table = :uts_compare


#cmd = "make mpi_run SRUN_FLAGS='--time=60' TARGET=pagerank.exe \
#                    PPN=%{ppn} \
#                    NNODE=%{nnode} \
#            GARGS='--num_starting_workers=%{num_threads} \
#                   --steal=1 \
#                   --chunk_size=%{chunk_size} \
#                   --aggregator_autoflush_ticks=%{autoflush_ticks} \
#                   --async_par_for_threshold=%{async_par_for_threshold} \
#                   --periodic_poll_ticks=%{poll_period} \
#                   --nnz_factor=%{nnz_factor} \
#                   --logN=%{logN} \
#                   --row_distribute=%{row_distribute}'"

def e( treestr ) 
    ENV[treestr]
end

cmd = "srun --resv-ports --partition=grappa --cpu_bind=verbose --task-prolog=.srunrc.upc \
-N%{nnode} \
-n%{nproc} \
./%{uts_upc_version} \
%{tree_args} \
-c %{chunk_size} \
-s %{steal} \
-i %{chunk_release_interval} \
-I %{bupc_poll_interval} \
-V %{vertices_size}"

params = {
    trial: [1,2,3],
    nnode: [4,8,12],
    chunk_size: [20,50],
    chunk_release_interval: [1,10],
    bupc_poll_interval: [1,10],
    ppn: [4,8,2,1],
    nproc: expr('nnode * ppn'),
    steal: [1],
    tree: ['T1L', 'T2L', 'T3L'],
    tree_args: expr('ENV[tree]'),
    uts_upc_version: ['uts-upc'],
    problem: ['uts']
}
params = {
    trial: [1],
    nnode: [1],
    chunk_size: [1],
    chunk_release_interval: [1],
    bupc_poll_interval: [1],
    ppn: [1],
    nproc: expr('nnode * ppn'),
    steal: [0],
    tree: ['T1', 'T2', 'T3'],
    tree_args: expr('ENV[tree]'),
    uts_upc_version: ['uts-mem-upc'],
    problem: ['uts'],
    vertices_size: [ENV['SIZE0']]
}


"""
Tree size = 102181082, tree depth = 13, num leaves = 81746377 (80.00%)
Wallclock time = 4.032 sec, performance = 25344315 nodes/sec (2112026 nodes/sec per PE)

Total chunks released = 370108, of which 362691 reacquired and 7417 stolen
Failed steal operations = 11499, Max stealStack size = 13023
Avg time per thread: Work = 3.739485, Search = 0.251081, Idle = 0.007587
                     Overhead = 0.023253, CB_Overhead = 0.010083
"""

parser = lambda{ |cmdout|
    records = {}

    # parse experiment specific results
    firstDict = /Tree size = (?<num_vertices>\d+), tree depth = (?<tree_depth>\d+), num leaves = (?<tree_num_leaves>\d+)/.match(cmdout).dictionize
    secondDict = /Wallclock time = (?<stock_runtime>\d+\.\d+)/.match(cmdout).dictionize
    
    if firstDict.empty? or secondDict.empty? then
        raise "Output string does not match"
    end

    records.merge!(firstDict)
    records.merge!(secondDict)

    records
}

run_experiments(cmd, params, db, table, &parser)
