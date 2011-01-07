#!/usr/bin/python
import sys
import os 

sys.path.append("../../exper_runner")
import loop_runexp_nowait

#os.putenv("GOMP_CPU_AFFINITY","0-15:2")
os.putenv("GOMP_CPU_AFFINITY","0-23:2")

#pfmon_counters = "UNC_DRAM_PAGE_CLOSE:CH0,SQ_FULL_STALL_CYCLES,OFFCORE_REQUESTS_SQ_FULL,UNC_GQ_CYCLES_FULL:READ_TRACKER,UNC_GQ_CYCLES_NOT_EMPTY:READ_TRACKER,UNC_CLK_UNHALTED"

#pfmon_counters = "L2_DATA_RQSTS:PREFETCH_MESI,L1D_CACHE_PREFETCH_LOCK_FB_HIT,L1D_PREFETCH:MISS,L1D_PREFETCH:REQUESTS,L1D_PREFETCH:TRIGGERS"
#pfmon_counters = "UNC_GQ_ALLOC:READ_TRACKER,UNC_GQ_CYCLES_FULL:READ_TRACKER,UNC_QMC_NORMAL_READS:CH0,UNC_QMC_NORMAL_READS:CH1,UNC_QMC_NORMAL_READS:CH2,UNC_DRAM_PAGE_MISS:CH0,UNC_DRAM_READ_CAS:CH0,UNC_DRAM_READ_CAS:AUTOPRE_CH0"
#pfmon_counters = "UNC_GQ_ALLOC:RT_LLC_MISS,UNC_GQ_CYCLES_FULL:READ_TRACKER,UNC_QHL_CYCLES_FULL:REMOTE,UNC_QHL_CYCLES_FULL:LOCAL,UNC_QMC_NORMAL_READS:ANY,UNC_DRAM_PAGE_MISS:CH0,UNC_DRAM_READ_CAS:CH0,UNC_DRAM_READ_CAS:AUTOPRE_CH0"
#pfmon_counters = "UNC_DRAM_PAGE_MISS:CH0,UNC_DRAM_READ_CAS:CH0,UNC_DRAM_READ_CAS:AUTOPRE_CH0,UNC_DRAM_REFRESH:CH0,UNC_DRAM_PRE_ALL:CH0,UNC_DRAM_PAGE_CLOSE:CH0"
#pfmon_counters = "UNC_GQ_ALLOC:READ_TRACKER,UNC_GQ_CYCLES_FULL:READ_TRACKER,UNC_QMC_NORMAL_READS:CH0,UNC_QMC_NORMAL_READS:CH1,UNC_QMC_NORMAL_READS:CH2,UNC_DRAM_PAGE_MISS:CH0,UNC_DRAM_READ_CAS:CH0,UNC_DRAM_READ_CAS:AUTOPRE_CH0"

#pfmon_counters = "UNC_QMC_BUSY:READ_CH0,UNC_QMC_BUSY:READ_CH1,UNC_QMC_BUSY:READ_CH2,UNC_QMC_NORMAL_READS:CH0,UNC_QMC_NORMAL_READS:CH1,UNC_QMC_NORMAL_READS:CH2,UNC_QHL_REQUESTS:LOCAL_READS,UNC_QHL_CYCLES_FULL:LOCAL"

#pfmon_counters = "UNC_GQ_ALLOC:PEER_PROBE_TRACKER,UNC_GQ_CYCLES_FULL:PEER_PROBE_TRACKER,UNC_GQ_CYCLES_NOT_EMPTY:PEER_PROBE_TRACKER,UNC_GQ_DATA:FROM_QPI,UNC_GQ_DATA:FROM_QMC,UNC_GQ_DATA:FROM_LLC,UNC_GQ_DATA:FROM_CORES_02,UNC_GQ_DATA:FROM_CORES_13,UNC_GQ_DATA:TO_QPI_QMC,UNC_GQ_DATA:TO_LLC,UNC_GQ_DATA:TO_CORES"
pfmon_counters = "UNC_GQ_ALLOC:PEER_PROBE_TRACKER,UNC_GQ_CYCLES_FULL:PEER_PROBE_TRACKER,UNC_GQ_CYCLES_NOT_EMPTY:PEER_PROBE_TRACKER,UNC_GQ_DATA:FROM_QPI,UNC_GQ_DATA:FROM_QMC,UNC_GQ_DATA:FROM_CORES_02,UNC_GQ_DATA:FROM_CORES_13,UNC_GQ_DATA:TO_QPI_QMC"#,UNC_GQ_DATA:TO_LLC"#,UNC_GQ_DATA:TO_CORES"

# L1D_CACHE_PREFETCH_LOCK_FB_HIT
# L1D_PREFETCH:MISS
# L1D_PREFETCH:REQUESTS
# L1D_PREFETCH:TRIGGERS
# L2_DATA_RQSTS:PREFETCH_E_STATE
# L2_DATA_RQSTS:PREFETCH_I_STATE
# L2_DATA_RQSTS:PREFETCH_M_STATE
# L2_DATA_RQSTS:PREFETCH_MESI
# L2_DATA_RQSTS:PREFETCH_S_STATE
# L2_LINES_OUT:PREFETCH_CLEAN
# L2_LINES_OUT:PREFETCH_DIRTY
# L2_RQSTS:PREFETCH_HIT
# L2_RQSTS:PREFETCH_MISS
# L2_RQSTS:PREFETCHES
# L2_TRANSACTIONS:PREFETCH

save_dir = ""
use_save_dir = False
if len(sys.argv) > 1:
	if sys.argv[1]=="-s":
		if len(sys.argv) == 3:
			save_dir = sys.argv[2]
		 	os.mkdir(save_dir)	
			use_save_dir = True
		else: raise Error("no save directory specified for option -s")
	else: raise Error("unrecognized option %s" % (sys.argv[1]))

if use_save_dir:
	print "using save directory %s" % (save_dir)
else:
	print "not using a save directory (to specify use option -s)"

pfmon_args = " -e %s --cpu-list 0 --system-wide -u -k" % pfmon_counters
#cmd = "taskset %s nice -15 ./linked_list.exe 24 %d %d -i"
cmd = "hugectl --heap -- numactl --membind=0 -- ./linked_list.exe -b 24 -t %d -c %d -l %d -g"

##cmd = "numactl --membind=0 -- ./linked_list.exe -b 24 -t %d -c %d -l %d -g"

results = []
#for num_threads, taskset in [(1,0x1), (2,0x5), (4,0x15), (8,0x55), (16,0x5555)]:

max_threads = 12
max_concurrent_reads = 4
green_list = [i for i in range(1,24+1)] + [32,37,41,43,47,53,59,63,64,65,71,97,111,123,127,128,131,139,149,151]

# build cpu list
cpus = range(0,max_threads,2)
cpus.extend(range(1,max_threads,2))

# run all experiments
for num_threads in range(1, max_threads+1):
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] )
    print num_threads, ":", cpu_list
    os.putenv("GOMP_CPU_AFFINITY", cpu_list)
    for concurrent_reads in range(1,max_concurrent_reads+1):
	for green_threads_per in green_list:
        	these_results = loop_runexp_nowait.run_experiment(pfmon_args, cmd % (green_threads_per,num_threads, concurrent_reads), True)
        	results.extend(these_results)
		if use_save_dir:
			# write just these results, so in case of crash we save work
			f = open('%s/S_24_%d_%d_%d.csv'%(save_dir,green_threads_per,num_threads,concurrent_reads), 'w')
			f.write( loop_runexp_nowait.dictlist_to_csv(these_results) )
			f.close()

# write all results to one csv
f = open('listtraversal-localfirst.csv','w')
f.write( loop_runexp_nowait.dictlist_to_csv(results) )
f.close()


