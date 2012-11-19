#!/usr/bin/env ruby
require '../../experiment_utils'


db = "spmv.db"
table = :spmv


cmd = "make mpi_run TARGET=spmv_mult.exe \
                    PPN=%{ppn} \
                    NNODE=%{nnode} \
            GARGS='--num_starting_workers=%{num_threads} \
                   --steal=1 \
                   --chunk_size=%{chunk_size} \
                   --aggregator_autoflush_ticks=%{autoflush_ticks} \
                   --async_par_for_threshold=%{async_par_for_threshold} \
                   --periodic_poll_ticks=%{poll_period} \
                   --nnz_factor=%{nnz_factor} \
                   --logN=%{logN} \
                   --row_distribute=%{row_distribute}'"


params = {
    num_threads:  [1024], #[128,32,64,78,104,256],#[256, 350, 512, 768, 1024, 128], #72,104
    chunk_size: [100],
    trial: [1,2,3],
    poll_period: [20000],
    autoflush_ticks: [2000000 ],
    ppn: [4],#[12,11,10,9,8,7,6,5,4,3,2,1],#[4, 3, 5, 6],# 7, 8],
    nnode: [12], #[12],# [8,4],#,12],#,12,10],
    nproc: expr('nnode * ppn'),
    nnz_factor: [4],#[16,4,10,20,6,100],
    logN: [21],#[6,16,17,18,19,20,21,22,23,24,25],
    doSteal: [1],
    async_par_for_threshold: [2],
    row_distribute: ['true']
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
