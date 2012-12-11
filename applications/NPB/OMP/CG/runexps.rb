#!/usr/bin/env ruby
require '../../../../experiment_utils'


db = "cg.db"
table = :omp_n02


cmd = "make run PROBLEM=%{problem} OMP_NUM_THREADS=%{nthreads}"

params = {
    trial: [1,2,3],
    problem: ['A','B','C','D'],
    nthreads:  [32],#[16,24,32,48,64], 
}

parser = lambda{ |cmdout|
    records = {}

    cgreg = /(?<key>[a-zA-Z\s\/]+)\s+=\s+(?<value>.+)/
    cmdout.scan(cgreg).each { |k,v|
        k = k.gsub(/\s+/,"_").gsub(/\//,"_per_")
        if v.match(/\d+\.\d+/) then 
            v = v.to_f
        elsif v.match(/\d+/) then
            v = v.to_i
        end
        records[k.to_sym] = v
    }

    records
}

run_experiments(cmd, params, db, table, &parser)
