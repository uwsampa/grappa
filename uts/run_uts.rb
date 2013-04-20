#!/usr/bin/env ruby
#require "/sampa/home/bdmyers/experiments/lib/experiments"
require "/sampa/home/bdmyers/experiments/experiments"
#require "/sampa/home/bdmyers/experiments/lib/parsers"
#require "/pic/people/holt225/dev/experiments/lib/experiments"
#require "experiments"
#require "parsers"


#db = "#{ENV['GRAPPA_DATA_PATH']}/#{ENV['GRAPPA_DATA_FN']}"
db = "data_dud.db"
table = :uts_lb

cmd = "make mpi_run SRUN_FLAGS='--time=10' \
                    TARGET=uts_grappa.exe \
                    PPN=%{procs_per_node} \
                    NNODE=%{num_places} \
                    GARGS='--num_starting_workers=%{num_threads} \
                           --load_balance=%{load_balance} \
                           --chunk_size=%{chunk_size} \
                           --global_queue_threshold=%{global_queue_threshold} \
                           --async_par_for_threshold=%{search_threshold} \
                           --vertices_size=%{vertices_size} \
                           --aggregator_autoflush_ticks=%{autoflush_ticks} \
                           --global_memory_use_hugepages=%{hugep} \
                           --periodic_poll_ticks=%{poll_period} \
                           -- $(%{tree}) -v 2' \
                    2>&1 | tee -a run.log"

# 2>&1 | tee -a run.log.%{procs_per_node}.%{num_places}.%{search_threshold}.%{num_threads}.%{chunk_size}"

params = {
    trial:              [1,2,3], 
    hugep:              ['true'],
    search_threshold:   [8],
    steal_idle_only:    ['false'],
    chunk_size:         [100],
    num_places:         [8], 
    num_threads:        [1024,1536], 
    procs_per_node:     [5], 
    poll_period:        [10000], #[10000,5000],
    autoflush_ticks:    expr('num_threads * 2000'), #[1000000, 2000000 ],
    tree:               ['T1L'], #['T1XL'],
    vertices_size:      [ENV['SIZEL']], #[ENV['SIZEXL']], #
    num_procs:          expr('num_places * procs_per_node'),
    load_balance:       ["steal","share"], #none,gq
    global_queue_threshold: [0]
}

parser = lambda{ |cmdout|
    p cmdout
    resultDict = {}
    
    dict_names = ["uts:",
                  "CommunicatorStatistics",
                  "TaskStatistics",
                  "TaskingSchedulerStatistics",
                  "AggregatorStatistics",
                  "SoftXMTStats",
                  "DelegateStats",
                  "CacheStats",
                  "IncoherentAcquirerStatistics",
                  "IncoherentReleaserStatistics",
                  "StealStatistics",
                  "GlobalQueueStatistics"]

    dict_names.each do |name|
        dmat = /#{name} \{([^{}]+:[^{}]+,|[^{}]+:[^{}]+)*\}/.match(cmdout)
        if dmat.nil?
            raise "unexpected output format '#{name}'"
        else
            dstr1 = dmat[0].sub(/^#{name} /,"") #remove identifier to make a proper dictionary; make into string
            dstr2 = dstr1.sub(/\n\d\d: /,"") #remove spurious newlines in logfiles
            p dstr2
            asDict = eval(dstr2)
            p asDict
            resultDict = resultDict.merge(asDict)
        end
    end

    resultDict
}

run_experiments(cmd, params, db, table, &parser)


