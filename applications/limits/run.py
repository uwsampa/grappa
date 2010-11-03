#!/usr/bin/python
import sys
import os 

sys.path.append("../../exper_runner")
import loop_runexp

os.putenv("GOMP_CPU_AFFINITY","0-15:2")

pfmon_counters = "UNC_DRAM_PAGE_CLOSE:CH0,SQ_FULL_STALL_CYCLES,OFFCORE_REQUESTS_SQ_FULL,UNC_GQ_CYCLES_FULL:READ_TRACKER,UNC_GQ_CYCLES_NOT_EMPTY:READ_TRACKER,UNC_CLK_UNHALTED"

pfmon_args = " -e %s --cpu-list 0 --system-wide -u -k" % pfmon_counters
#cmd = "taskset %s nice -15 ./linked_list.exe 24 %d %d -i"
cmd = "nice -15 ./linked_list.exe 24 %d %d -i"

results = []
#for num_threads, taskset in [(1,0x1), (2,0x5), (4,0x15), (8,0x55), (16,0x5555)]:
for num_threads in [1, 2, 4, 8, 16]:
    for concurrent_reads in [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]:
        #these_results = loop_runexp.run_experiment(pfmon_args, cmd % (taskset, num_threads, concurrent_reads), True)
        these_results = loop_runexp.run_experiment(pfmon_args, cmd % (num_threads, concurrent_reads), True)
        results.extend(these_results)

f = open('results.csv','w')
f.write( loop_runexp.dictlist_to_csv(results) )
f.close()


