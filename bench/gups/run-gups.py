import pexpect
import os
import sys

sys.path.append("../../exper_runner")
import loop_runexp_nowait

max_threads = 48

# TODO args/options
save_dir = sys.argv[1]
results_file = sys.argv[1]+"/"+sys.argv[1]+".csv"

os.mkdir(save_dir)

numanodes = {0:[ 0, 4,8,12,16,20],
             1:[24,28,32,36,40,44],
             2:[ 3, 7,11,15,19,23],
             3:[27,31,35,39,43,47],
             4:[ 2, 6,10,14,18,22],
             5:[26,30,34,38,42,46],
             6:[ 1, 5, 9,13,17,21],
             7:[25,29,33,37,41,45]}

#TODO generalize
localcores = numanodes[2]
remotecores = numanodes[4]
cpus = localcores+remotecores
#cpus = [0,2,4,8,10,12,1,3,5,7,9,11]


cmd_template = "likwid-pin -c %s numactl --%s=%s ./gupsmc -f %d -u %d -c %d"
#cmd_template = "hugectl --heap ./gups -f %d -u %d -c %d"
#cmd_template = "hugectl --heap numactl --%s=%s ./gups -f %d -u %d -c %d"

results = []

for num_threads in [6]:#range(1, 6+1):
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] )
    print num_threads,":",cpu_list
    #os.putenv("GOMP_CPU_AFFINITY", cpu_list)
    cmask = ",".join( [str(i) for i in cpus[0:num_threads]] )
    #numa = ''
    #if num_threads <= 6: numa = '0'
    #else: numa = '0,1'
    numa = '2'

    for fieldsize in range(0,27+1):
        for num_ups in [24]:
            for alloc in ['membind']:#['interleave','membind']:
                #for nrandom in [True]:#[True, False]:
                nrandom = True
                if not nrandom: cmd+=' -n'
                for huge in [True,False]:
                    
                    for atomic in ['partition']:#['delegate']:#[True, False, 'delegate']:
                        cmd = cmd_template
                        if huge: cmd+=' -H'
                        if atomic is True: cmd+=' -a'
                        if atomic is 'delegate': cmd+=' -d'
                        if atomic is 'partition': cmd+=' -p'

                        this_cmd = cmd%(cmask,alloc,numa,fieldsize,num_ups,num_threads)

                        these_results = loop_runexp_nowait.run_experiment('', this_cmd, True)
                        results.extend(these_results)
                        if save_dir is not None:
                                f = open('%s/GUPS_f%d_u%d_c%d_%s_a%s_r%s'%(save_dir,fieldsize,num_ups,num_threads,alloc,str(atomic),str(not nrandom)),'a')
                                f.write( loop_runexp_nowait.dictlist_to_csv(these_results) )
                                f.close()

# write all results to one csv
f = open(results_file, 'w')
f.write (loop_runexp_nowait.dictlist_to_csv(results))
f.close()


