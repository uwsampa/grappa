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
	#cmd_proc.logfile = sys.stdout

	while (cmd_proc.isalive()):
		results = {}
		# wait for "ready to start new experiment" signal from experiment program
		try:
			cmd_proc.expect(EXP_START)
		except pexpect.EOF:	
			# I believe this works; could make it simpler with adding
			# a ExperimentRunner::doneWithExperiments to signify end,
			# but this another thing that would be added to API
		
			print cmd_proc.before # print the rest of the output that was before EOF
			break;


		# print stuff printed by cmd_proc after last experiment (or after setup if first iteration)
		print cmd_proc.before

		# start monitor and tell experiment to run
		perfmon_proc = None
		if system_wide:
			perfmon_proc = pexpect.spawn('pfmon '+pfmargs)
		else:
			perfmon_proc = pexpect.spawn('pfmon --attach-task='+str(cmd_proc.pid)+' '+pfmargs)

		# let experiment run
		cmd_proc.sendline('y')	
		cmd_proc.expect('y')  # get rid of sent line from output

		# wait for "experiment is done" signal from experiment program
		cmd_proc.expect(EXP_DONE)

		# terminate monitor
		if system_wide:
			# can stop session with ENTER
			perfmon_proc.sendline('')
		else:
			# can stop with SIGINT
			perfmon_proc.kill(pexpect.signal.SIGINT)


		# tell experiment program to continue
		cmd_proc.sendline('z')
		cmd_proc.expect('z')  # get rid of sent line from output

		## capture results from experiment process as dictionary
		cmd_proc.expect("({.*})")
		results = dict( eval( cmd_proc.match.group(1) ) )

		# capture output of monitor for this experiment
		perfmon_results = perfmon_proc.read()
		print perfmon_results
		r = re.compile("^C?P?U?[0-9]*\s+([0-9]*) (\S+).*$")
		for i in perfmon_results.splitlines():
			m = r.search(i)
			if m is not None:
				#print m.group(2), m.group(1)
				results[m.group(2)] = m.group(1)
		all_results.append(results)

		## print stuff printed by cmd_proc between start and stop
		print cmd_proc.after

	# print rest of cmd_proc output (if none then '' is printed)
	print cmd_proc.read()
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
			str += "%s, " % d[c]

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
	
