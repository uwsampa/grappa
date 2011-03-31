import pexpect
import sys
import re

EXP_START = 'reqb';
EXP_DONE  = 'reqe';

# returns list of result dictionaries
def run_experiment(pfmargs, cmd, system_wide):
    all_results = []

	# start experiment program, to let it set up
    cmd_proc = pexpect.spawn(cmd)
    cmd_proc.timeout = 240
    #cmd_proc.logfile = sys.stdout

    while (True):
        results = {}
        # wait for "ready to start new experiment" signal from experiment program
        try:
	        ## capture results from experiment process as dictionary
            cmd_proc.expect("({.*})")
        except pexpect.EOF:		
            #print cmd_proc.before # print the rest of the output that was before EOF
            break;

        results = dict( eval( cmd_proc.match.group(1) ) )
    
        all_results.append(results)

		## print stuff printed by cmd_proc between start and stop
		#print cmd_proc.after


	# hacky
	# only one likwid output so copy the results into all columns
#	likres = []
#	m = re.search(re.compile("(Data cache misses)\s+\|\s+(\d+(\.\d+)?)"), cmd_proc.before)
#	likres.append((m.group(1), m.group(2)))
#
#	m = re.search(re.compile("(Data cache miss rate)\s+\|\s+(\d+(\.\d+)?)"), cmd_proc.before)
#	likres.append((m.group(1), m.group(2)))	
#
#	m = re.search(re.compile("(Miss ratio)\s+\|\s+(\d+(\.\d+)?)"), cmd_proc.before)
#	likres.append((m.group(1), m.group(2)))
#
#	for res in all_results:
#		for ele in likres:
#			res[ele[0]] = ele[1]

    cmd_proc.read()
    return all_results

def dictlist_to_csv(dl):
	str = ""
	# create column list
	columns_set = set()
	for d in dl:
		for k in d.keys():
			columns_set.add(k)

	# sort column list
	columns = sorted(columns_set)

	# create column list
	str = "%s," % (', '.join(columns))

	# add data
	for d in dl:
		str += "\n"	
		for c in columns:
			try:
				str += "%s, " % d[c]
			except KeyError:
				str += "0, "

	return str
			
			

if __name__ == '__main__':
	pfmargs = sys.argv[1]
	cmd = sys.argv[2]
	system_wide = False
	if len(sys.argv) == 4:
		if sys.argv[3] == '-s':
			system_wide = True
	results = run_experiment(pfmargs, cmd, system_wide)
	print dictlist_to_csv(results)
	
