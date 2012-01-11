import pexpect
import sys
import os

sys.path.append("../../../exper_runner")
import loop_runexp_nowait as lr

output_name = sys.argv[1]
verbose = int(sys.argv[2])

try:
    os.mkdir(output_name)
except OSError as e:
    if e.args[0]==17:  #file exists
        print 'directory exists: %s'%(output_name)
        while(True):
            answer = raw_input('continue? y/n')
            if answer=='y':
                break
            else:
                exit(1)

output_backups = output_name
output_csv = output_name+".csv"


num_simTasks = 64

num_cores = 10


cmd = 'mpirun -np %d -host %s ./TreeGen %s -n %d -T %d -c %d -i %d'

import re
brackets = re.compile('({.*})')

threshold = 0

all_results = []
for policy in [6]: #[0,1,4]:
    for num_nodes in [2,4,6,8,16,32]:
        for task_size in [32, 48, 64, 96, 128, 256, 512, 1024]:
            for window_size in [3*task_size,6*task_size,8*task_size]:
                this_cmd = cmd % (input_path, num_nodes, policy, num_simTasks, num_cores, task_size, window_size, threshold)
                print '>',this_cmd
                cmd_proc = pexpect.spawn(this_cmd)
                cmd_proc.timeout = 100000
                res = None
                try:
                    if verbose==1:
                       rr = cmd_proc.readline()
                       while rr!='':
                           print rr
                           m = brackets.match(rr)
                           if m:
                               res = dict(eval(m.group(1)))
                               break
                           rr = cmd_proc.readline()
                    else:
                        cmd_proc.expect('({.*})')
                        res = dict(eval(cmd_proc.match.group(1)))
                        res['window_size'] = window_size
                        res['threshold'] = threshold
                except Exception as e:
                    print '->failed because',e.message()
                    cmd_proc.close()
                    continue    
                     
                
                all_results.append(res)
        
                #write to disk in case of later failure
                bkfile = open(output_backups+'/bk_%d_%d_%d_%d_%d.csv'%(num_nodes, policy, num_simTasks, num_cores, task_size), 'w')
                bkfile.write(lr.dictlist_to_csv([res]))
                bkfile.close()


output_str = lr.dictlist_to_csv(all_results)
output_file = open(output_csv, 'a')
output_file.write('\n--new sim--\n')
output_file.write(output_str)
output_file.close()



