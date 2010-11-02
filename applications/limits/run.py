import loop_runexp


pfmon_counters = "UNC_DRAM_PAGE_CLOSE:CH0,SQ_FULL_STALL_CYCLES,OFFCORE_REQUESTS_SQ_FULL,UNC_GQ_CYCLES_FULL:READ_TRACKER,UNC_GQ_CYCLES_NOT_EMPTY:READ_TRACKER,UNC_CLK_UNHALTED"

pfmon_args = " -e %s --cpu-list 0 --system-wide -u -k" % pfmon_counters
cmd = "GOMP_CPU_AFFINITY='0-15:2' nice -15 ./linked_list.exe 24 %d %d -i"

results = []
for num_threads in [1, 2, 4, 8]:
    for concurrent_reads in [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]:
        these_results = loop_runexp.run_experiment(pfmon_args, cmd % (num_threads, concurrent_reads), True)
        results.extend(these_results)

print loop_runexp.dictlist_to_csv(results)

