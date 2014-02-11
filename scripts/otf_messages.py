#!/usr/bin/python

####################################################################################################
#
# Extracts the messages from the OTF dump file is the specified location
# Parameters:
#    1. The path to the original .otf file for analysis
#    2. The target location for the message dump file
#    3. The number of processes per node
#    4. The number of processes per core
#
# Message dump file logs sent message to file based on time they were sent in the trace.
# 
####################################################################################################

import os
import sys
import string

HOST_NAME_PREFIX = "n"                    # the prefix of the cluster node names such as "n72"
GROUP_NAME_STRING = "name \"" + HOST_NAME_PREFIX # search string to identify processes
TIME_TICKS_STRING = "TicksPerSecond"      # search string to determine ticks per second
SEND_MESSAGE_STRING = "SendMessage"       # search string to identify send message events
RECEIVE_MESSAGE_STRING = "ReceiveMessage" # search string to identify receive message events
DEF_PROCESS_GROUP = "DefProcessGroup"     # process group definition string
DEF_TIMER_RESOLUTION = "DefTimerResolution"# identify timer resolution string
PROCESS_NUMBER_STRING = "procs"              # identifies process prefix in group def line

####################################################################################################
#
# Parse the command line arguments from the file
# 1. The path to original .otf file
# 2. The name of message file to be generated
#
####################################################################################################

# basic argument validation
args = sys.argv
if (len(args) != 5):
    print "Error: Expected 4 args. Got " + str(len(args) - 1)
    print "Example Usage: ./otf_message.py <tracefile.otf> <logfile> <ppn> <nnode>"
    exit(-1)
else:
    pass

OTF_FILE = args[1]
MSG_FILE = args[2]
PPN = args[3]
NNODE = args[4]

# validate that you got an otf file
otf_ext = OTF_FILE[-4:]
if (otf_ext.lower() == ".otf"):
    pass
else:
    print "Error loading: " + OTF_FILE + " - file extension does not match .otf"
    exit(-1)

# dump the otf file to a temporary dump
DUMP_FILE = OTF_FILE[:-4] + ".dump"
command = "otfdump " + OTF_FILE + " > " + DUMP_FILE
os.system(command) 

####################################################################################################
#
# Open the OTF file and extract time and message send events
# 1. Extract number of ticks per second
# 2. Extract message events
#
####################################################################################################

TICKS_PER_SECOND = None
NODE_PROCESSES = dict()

# scan the file to populate the parameters
file = open(DUMP_FILE, "r")

for line in file:
    # search for the number of ticks for second - may be multiple definitions
    if (line.find(DEF_TIMER_RESOLUTION) != -1 and line.find(TIME_TICKS_STRING) != -1):
        ticks_per_second = line[line.find(TIME_TICKS_STRING) + len(TIME_TICKS_STRING):]
        ticks_per_second = int(ticks_per_second.strip(" \t\n"))
        if (TICKS_PER_SECOND != None):
            assert(ticks_per_second == TICKS_PER_SECOND)
        TICKS_PER_SECOND = ticks_per_second

    # search for the process group definition to extract core locality
    elif (line.find(DEF_PROCESS_GROUP) != -1 and line.find(GROUP_NAME_STRING) != -1):
        # parse the node number
        node_start_index = line.find(GROUP_NAME_STRING) + len(PROCESS_NUMBER_STRING)
        tail_string = line[node_start_index:]
        split_string = tail_string.split(",")
        assert(len(split_string) > 0)
        node = split_string[0].strip("\"")

        # parse the process ids associated with the node
        ids_start_index = line.find(PROCESS_NUMBER_STRING) + len(PROCESS_NUMBER_STRING)
        assert(ids_start_index != -1 and ids_start_index < len(line))
        ids = line[ids_start_index:]
        ids = ids.strip(" \t\n,")
        ids = ids.split(",")
        
        # log the ids for the node
        assert(not node in NODE_PROCESSES.keys())
        NODE_PROCESSES[node] = ids

assert(TICKS_PER_SECOND != None)
assert(len(NODE_PROCESSES.keys()) > 0)
print "Ticks per second: " + str(TICKS_PER_SECOND)
print NODE_PROCESSES
