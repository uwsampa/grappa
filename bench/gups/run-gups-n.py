import pexpect
import os
import sys

def taskset(num):
    return {1:"0x1",
            2:"0x5",
            3:"0x15",
            4:"0x55",
            5:"0x155",
            6:"0x555",
            7:"0x557"}[num]

sys.path.append("../../exper_runner")
import loop_runexp_nowait

max_threads = 48

# TODO args/options
save_dir = sys.argv[1]
results_file = sys.argv[1]+"/"+sys.argv[1]+".csv"

os.mkdir(save_dir)

numanodes = {0:[ 0, 2, 4, 6, 8, 10],
             1:[ 1, 3, 5, 7, 9, 11]}

localcores = numanodes[0]
remotecores = numanodes[1]
cpus = localcores+remotecores


cmd_template = "taskset %s numactl --%s=%s ./gupsn -f %d -u %d -c %d"

#cmd_template = "hugectl --heap numactl --%s=%s ./gupsn -f %d -u %d -c %d"
#cmd_template = "hugectl --heap ./gupsn -f %d -u %d -c %d"

results = []

#for num_threads in range(1, 12+1):
for num_threads in range(1,6+1): 
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] ) 
    print num_threads,":",cpu_list
    #os.putenv("GOMP_CPU_AFFINITY", "0 2 4 6 8 10")    
    #already in code
    
    #numa = ''
    #if num_threads <= 6: numa = '0'
    #else: numa = '0,1'
    numa = '0'

    for fieldsize in range(0,28+1):#[12,13,14,15,16,18,19,21,23,25,26]:
        for num_ups in [24]*3:
            for alloc in ['membind']:#['interleave','membind']:
                for nrandom in [True]:#[True, False]:

                    for atomic in ['nonatomic','atomic','delegate']:#['partition','atomic','nonatomic','delegate']:
                        cmask = taskset(num_threads)
                        #os.putenv("GOMP_CPU_AFFINITY", cpu_list)
                        
                        cmd = cmd_template
                        if atomic=='atomic': cmd +=' -a'
                        if atomic=='delegate': 
                            cmd+=' -d'
                            cmask = taskset(num_threads+1)
                           # os.putenv("GOMP_CPU_AFFINITY", " ".join( [str(i) for i in cpus[0:num_threads+1]] ))
                        if atomic=='partition': cmd+=' -p'
                        if not nrandom: cmd+=' -n'

                        #alloc = 'free'

                        this_cmd = cmd%(cmask,alloc,numa,fieldsize,num_ups,num_threads)
                        #this_cmd = cmd%(fieldsize,num_ups,num_threads)
                        
                        these_results = loop_runexp_nowait.run_experiment('', this_cmd, True)
#                        for resdict in these_results: resdict['cpu_pin'] = x
                   
                        results.extend(these_results)
                        if save_dir is not None:
                                f = open('%s/GUPS_f%d_u%d_c%d_%s_a%s_r%s'%(save_dir,fieldsize,num_ups,num_threads,alloc,str(atomic),str(not nrandom)),'a')
                                f.write( loop_runexp_nowait.dictlist_to_csv(these_results) )
                                f.close()

# write all results to one csv
f = open(results_file, 'w')
f.write (loop_runexp_nowait.dictlist_to_csv(results))
f.close()


