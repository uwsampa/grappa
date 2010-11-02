import pexpect
import sys


EXP_START = 'reqb';
EXP_DONE  = 'reqe';

pfmargs = sys.argv[1]
cmd = sys.argv[2]
system_wide = False

if len(sys.argv) == 4:
	if sys.argv[3] == '-s':
		system_wide = True
	

# start experiment program, to let it set up
cmd_proc = pexpect.spawn(cmd)

while (cmd_proc.isalive()):
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

	# print stuff printed by cmd_proc between start and stop
	# TODO: do we want to store (ie to print all later) instead of print?
	print cmd_proc.before

	# terminate monitor
	if system_wide:
		# can stop session with ENTER
		perfmon_proc.sendline('')
	else:
		# can stop with SIGINT
		perfmon_proc.kill(pexpect.signal.SIGINT)


	# print output of monitor for this experiment
	print perfmon_proc.read()

	# tell experiment program to continue
	cmd_proc.sendline('z')
	cmd_proc.expect('z')  # get rid of sent line from output


# print rest of cmd_proc output (if none then '' is printed)
print cmd_proc.read()
