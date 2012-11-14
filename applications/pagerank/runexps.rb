#!/usr/bin/env ruby
require '../../experiment_utils'


db = "spmv.db"
table = :spmv


cmd = "make mpi_run TARGET=sparsematrix_graph.exe PPN=%{procs_per_node} NPROC=%{num_places} GARGS='--num_starting_workers=%{num_threads} --steal=1 --chunk_size=%{chunk_size} --aggregator_autoflush_ticks=%{autoflush_ticks} --periodic_poll_ticks=%{poll_period} --nnz_factor=%{nnz_factor} --logN=%{logN}'"


params = {
    num_threads:  [1024], #[128,32,64,78,104,256],#[256, 350, 512, 768, 1024, 128], #72,104
    chunk_size: [100],
    trial: [1,2,3],
    poll_period: [20000],
    autoflush_ticks: [2000000 ],
    procs_per_node: [4,5],#[12,11,10,9,8,7,6,5,4,3,2,1],#[4, 3, 5, 6],# 7, 8],
    num_places: [12], #[12],# [8,4],#,12],#,12,10],
    num_procs: expr('num_places * procs_per_node'),
    nnz_factor: [6,10,16],
    logN: [6,16,17,18],
    doSteal: [1]
}

parser = lambda{ |cmdout|
    records = {}

    # parse Grappa STATS
    records.merge!($json_plus_fields_parser.call(cmdout))

    # parse experiment specific results
    exp_dict_str = /MULT\{([^{}]+:[^{}]+,|[^{}]+:[^{}]+)*\}/.match(cmdout)
    if exp_dict_str.nil? then
        raise "unexpected output format for MULT{}"
    else
        exp_dict_str = exp_dict_str[0].sub(/^MULT/,"") # remove identifier
        exp_dict = JSON.parse(exp_dict_str)
        exp_syms = {}
        exp_dict.each { |k,v| exp_syms[k.downcase.to_sym] = v }
        records.merge!(exp_syms)
    end

    records
}

run_experiments(cmd, params, db, table, &parser)
