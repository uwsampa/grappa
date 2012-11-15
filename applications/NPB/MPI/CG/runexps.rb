#!/usr/bin/env ruby
require '../../../../experiment_utils'


db = "cg.db"
table = :mpi


cmd = "make run TARGET=../bin/cg.%{problem}.%{nproc} PPN=%{ppn} NNODE=%{nnode}"

params = {
    trial: [1,2,3],
    problem: ['D','B','C','A'],
    nproc: [64],
    nnode: [8],
    ppn: expr('nproc / nnode')
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
    if records.length == 0 then
        raise "no records found"
    end

    records
}

run_experiments(cmd, params, db, table, &parser)
