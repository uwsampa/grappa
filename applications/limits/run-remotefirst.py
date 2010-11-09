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

max_threads = 16
max_concurrent_reads = 12

# build cpu list
cpus = range(1,max_threads,2)
cpus.extend(range(0,max_threads,2))

for num_threads in range(1, max_threads+1):
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] )
    print num_threads, ":", cpu_list
    os.putenv("GOMP_CPU_AFFINITY", cpu_list)
    for concurrent_reads in range(1,max_concurrent_reads+1):
        these_results = loop_runexp.run_experiment(pfmon_args, cmd % (num_threads, concurrent_reads), True)
        results.extend(these_results)

f = open('listtraversal-remotefirst.csv','w')
f.write( loop_runexp.dictlist_to_csv(results) )
f.close()


