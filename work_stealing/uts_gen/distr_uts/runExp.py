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


cmd = 'mpirun -np %d -host "%s" ./TreeGen %s -n %d -T %d -c %d -i %d'

hosts = ['localhost']
hoststring = ",".join(hosts)

num_cores = 1

import re
brackets = re.compile('({.*})')

threshold = 0

trees = {'T1':'-t 1 -a 3 -d 10 -b 4 -r 19',
         'T3':'-t 0 -b 2000 -q 0.124875 -m 8 -r 42'}

all_results = []
for treename in ['T1', 'T3']:
    for num_processes in [2,3,4,5,6]:
        for num_threads_per_core in [1,2,4,8,16]:
            for chunksize in [1,5,10,20]: 
                for cbarrier_interval in [1,2,16]:
                
                    this_cmd = cmd % (num_processes, hoststring, trees[treename], num_cores, num_threads_per_core, chunksize, cbarrier_interval)
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
                    except Exception as e:
                        print '->failed because',e.message()
                        cmd_proc.close()
                        continue    
                         
                    res['tree'] = treename                    
                    all_results.append(res)
            
                    #write to disk in case of later failure
                    bkfile = open(output_backups+'/bk_%d_%s_%d_%d_%d.csv'%(num_processes,treename, num_threads_per_core, chunksize, cbarrier_interval), 'w')
                    bkfile.write(lr.dictlist_to_csv([res]))
                    bkfile.close()


output_str = lr.dictlist_to_csv(all_results)
output_file = open(output_csv, 'a')
output_file.write('\n--new sim--\n')
output_file.write(output_str)
output_file.close()



