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

numanodes = {0:[ 0, 2, 4, 6, 8, 10],
             1:[ 1, 3, 5, 7, 9, 11]}

localcores = numanodes[0]
remotecores = numanodes[1]
cpus = localcores+remotecores


cmd_template = "numactl --membind=%s ./twowaydel %d %d %d" # PER-NUM  CORES   BUFSIZE

#cmd_template = "hugectl --heap numactl --%s=%s ./gupsn -f %d -u %d -c %d"
#cmd_template = "hugectl --heap ./gupsn -f %d -u %d -c %d"

results = []

#for num_threads in range(1, 12+1):
for num_threads in range(2,6+1): 
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] )
    print num_threads,":",cpu_list
    #already in code
    os.putenv("GOMP_CPU_AFFINITY", cpu_list)
    
    #numa = ''
    #if num_threads <= 6: numa = '0'
    #else: numa = '0,1'
    numa = '0'
    
    qsize = 1<<15

    for per_num in [10000000]:
        for bufsize in range(1,16+1):
            unmake_proc = pexpect.spawn("make clean");
            print unmake_proc.read()
            unmake_proc.close()
            make_proc = pexpect.spawn("make -e CMDL_FLAGS='-DCHUNK_SIZE=%d -DQSIZE=%d'"%(bufsize,qsize));
            print make_proc.read()
            make_proc.close()

            for trial in [1,2,3]:
                this_cmd = cmd_template%(numa,per_num,num_threads,bufsize)
                     
                these_results = loop_runexp_nowait.run_experiment('', this_cmd, True)
                results.extend(these_results)
                if save_dir is not None:
                    f = open('%s/TWOWAYDEL_n%d_c%d_b%d_%d'%(save_dir,per_num,num_threads,bufsize,trial),'w')
                    f.write( loop_runexp_nowait.dictlist_to_csv(these_results) )
                    f.close()

# write all results to one csv
f = open(results_file, 'w')
f.write (loop_runexp_nowait.dictlist_to_csv(results))
f.close()


