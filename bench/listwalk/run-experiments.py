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



## parse options
DEFAULT_BITSIZE = 24
DEFAULT_FILENAME =  'listtraversal-localfirst.csv'

from optparse import OptionParser
parser = OptionParser()
parser.add_option("-d", "--savedir", dest="save_dir",
                  help="write individual experiment results to files in DIR", metavar="DIR")
parser.add_option("-b", "--size", dest="bit_size",
                  help="total size for each run (2^NUM total list nodes) (default: %s)"%(DEFAULT_BITSIZE), metavar="NUM",
		  default=DEFAULT_BITSIZE)
parser.add_option("-u", "--hugepages", dest="use_hugepages",
                  help="use huge pages (default: off)", action="store_true", default=False)
parser.add_option("-f", "--resultfile", dest="results_file", default=DEFAULT_FILENAME,
                  help="write final results to FILE (default: '%s')"%(DEFAULT_FILENAME), metavar="FILE")
parser.add_option("-r", "--remotefirst", dest="run_remote_first",
		  help="add remote cores then local cores (default: off, i.e. add local cores before remote cores)",
                  action="store_true", default=False)
parser.add_option("-n", "--no-green-threads", dest="use_green_threads", action="store_false", default=True,
		  help="do not use green threads in experiments")

(options, args) = parser.parse_args()

save_dir = options.save_dir
bit_size = int(options.bit_size)
use_hugepages = options.use_hugepages
results_file = options.results_file
run_remote_first = options.run_remote_first
use_green_threads = options.use_green_threads
##


# print save directory confirmation
if save_dir is not None:
	print "using save directory %s" % (save_dir)
	os.mkdir(save_dir)
else:
	print "not using a save directory (to specify use option -s)"

# pfmon args
pfmon_args = " -e %s --cpu-list 0 --system-wide -u -k" % pfmon_counters

# set command template for running experiments
cmd = "hugectl --heap -- numactl --membind=0 -- ./linked_list.exe -b %d -t %d -c %d -l %d"
if not use_hugepages:
	# use this command if not using huge pages
	cmd = "numactl --membind=0 -- ./linked_list.exe -b %d -t %d -c %d -l %d"

# add -g option if using green threads
if use_green_threads:
	cmd = cmd+" -g"

# list of experiment results
results = []

max_threads = 12
max_concurrent_reads = 4

green_list = [i for i in range(1,24+1)] # + [32,37,41,43,47,53,59,63,64,65,71,97,111,123,127,128,131,139,149,151]
if not use_green_threads:
	# cannot have any argument for -t except 1 if green threads off
	green_list = [1]

# build cpu list
cpus = []
local_cores = range(0,max_threads,2)
remote_cores = range(1,max_threads,2)
if not run_remote_first:  # add local cores first
    cpus.extend(local_cores)
    cpus.extend(remote_cores) 
else:                    # add remote cores first
    cpus.extend(remote_cores)   
    cpus.extend(local_cores)

# run all experiments
for num_threads in range(1, max_threads+1):
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] )
    print num_threads, ":", cpu_list
    os.putenv("GOMP_CPU_AFFINITY", cpu_list)
    for concurrent_reads in range(1,max_concurrent_reads+1):
	for green_threads_per in green_list:
        	these_results = loop_runexp_nowait.run_experiment(pfmon_args, cmd % (bit_size,green_threads_per,num_threads, concurrent_reads), True)
        	results.extend(these_results)
		if save_dir is not None:
			# write just these results, so in case of crash we save work
			f = open('%s/S_%d_%d_%d_%d.csv'%(save_dir,bit_size,green_threads_per,num_threads,concurrent_reads), 'w')
			f.write( loop_runexp_nowait.dictlist_to_csv(these_results) )
			f.close()

# write all results to one csv
f = open(results_file,'w')
f.write( loop_runexp_nowait.dictlist_to_csv(results) )
f.close()


