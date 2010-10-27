import pexpect
import sys


pfmargs = sys.argv[1]
cmd = sys.argv[2]
system_wide = False

if len(sys.argv) == 4:
	if sys.argv[3] == '-s':
		system_wide = True
	

# start experiment, to let it set up
cmd_proc = pexpect.spawn(cmd)

# wait for setup finished
cmd_proc.expect('reqb')

# start monitor and tell experiment to run
perfmon_proc = None
if system_wide:
	perfmon_proc = pexpect.spawn('pfmon '+pfmargs)
else:
	perfmon_proc = pexpect.spawn('pfmon --attach-task='+str(cmd_proc.pid)+' '+pfmargs)

cmd_proc.sendline('y')

# wait for experiment to finish
cmd_proc.expect('reqe')

# terminate monitor
if system_wide:
	# can stop session with ENTER
	perfmon_proc.sendline('')
else:
	# can stop with SIGINT
	perfmon_proc.kill(pexpect.signal.SIGINT)

# tell experiment to continue
cmd_proc.sendline('z')

# TODO use output of monitor, right now just printing
print perfmon_proc.read()


