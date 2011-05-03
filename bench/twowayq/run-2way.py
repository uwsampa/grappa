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

#numanodes = {0:[ 0, 2, 4, 6, 8, 10],
#             1:[ 1, 3, 5, 7, 9, 11]}
numanodes = {0:[ 0, 4,8,12,16,20],
             1:[24,28,32,36,40,44],
             2:[ 3, 7,11,15,19,23],
             3:[27,31,35,39,43,47],
             4:[ 2, 6,10,14,18,22],
             5:[26,30,34,38,42,46],
             6:[ 1, 5, 9,13,17,21],
             7:[25,29,33,37,41,45]}

localcores = numanodes[0]
remotecores = numanodes[1]
cpus = localcores+remotecores


cmd_template = "numactl --%s=%s ./twowayq  %d %d %d" # PER-NUM  CORES   BUFSIZE

#cmd_template = "hugectl --heap numactl --%s=%s ./gupsn -f %d -u %d -c %d"
#cmd_template = "hugectl --heap ./gupsn -f %d -u %d -c %d"

results = []

#for num_threads in range(1, 12+1):
for num_threads in [2,4,6]:#,8,10,12]: 
    cpu_list = " ".join( [str(i) for i in cpus[0:num_threads]] )
    print num_threads,":",cpu_list
    #already in code
    os.putenv("GOMP_CPU_AFFINITY", cpu_list)
    
    #numa = ''
    #if num_threads <= 6: numa = '0'
    #else: numa = '0,1'
    numa = '0'

    for per_num in [10000000]:
        for bufsize in range(1,16+1):
            unmake_proc = pexpect.spawn("make clean");
            print unmake_proc.read()
            unmake_proc.close()
            chunks=[1,2,4,8,16]
            chchunk = 1
            for c in chunks:   # forcing to compile with power of 2 chunk size
                if bufsize<=c:
                    chchunk = c
                    break
            make_proc = pexpect.spawn("make -e CMDL_FLAGS='-DCHUNK_SIZE=%d'"%(chchunk));
            print make_proc.read()
            make_proc.close()

            for trial in [1,2,3]:
                this_cmd = cmd_template%('membind', numa, per_num,num_threads,bufsize)
                     
                these_results = loop_runexp_nowait.run_experiment('', this_cmd, True)
                results.extend(these_results)
                if save_dir is not None:
                    f = open('%s/TWOWAYQ_n%d_c%d_b%d_%d'%(save_dir,per_num,num_threads,bufsize,trial),'w')
                    f.write( loop_runexp_nowait.dictlist_to_csv(these_results) )
                    f.close()

# write all results to one csv
f = open(results_file, 'w')
f.write (loop_runexp_nowait.dictlist_to_csv(results))
f.close()


