#!/usr/bin/python

####################################################################################################
#
# Extracts the messages from the OTF dump file is the specified location
# Parameters:
#    1. The path to the original .otf file for analysis
#    2. The target location for the message dump file
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
PROCESS_NUMBER_STRING = "procs"            # identifies process prefix in group def line

SENDER_TAG = "sender"    # tag identifying sender process
RECEIVER_TAG = "receiver"# tag identifying receiver process
LENGTH_TAG = "length"    # tag identifying length of sent message
SEND_FLAG = "SEND"       # flag indicating message event was a send
RECEIVE_FLAG = "RECEIVE" # flag indicating message event was a receive

####################################################################################################
#
# Parse the command line arguments from the file
# 1. The path to original .otf file
# 2. The name of message file to be generated
#
####################################################################################################

# basic argument validation
args = sys.argv
if (len(args) != 3):
    print "Error: Expected 2 args. Got " + str(len(args) - 1)
    print "Example Usage: ./otf_message.py <tracefile.otf> <logfile>"
    exit(-1)
else:
    pass

OTF_FILE = args[1]
MSG_FILE = args[2]

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
# 1. Extract number of ticks per second, process association with nodes
# 2. Extract message events
#
# File Output Format:
# <time>, <sender node>, <sender process>, <receiver node> , <receiver process>, <message size>, <SEND FLAG>
# - SEND FLAG indicates if the event is a message send or receiver event
# - Receives currently not logged
#
####################################################################################################

print "[Info] Processing OTF File: " + OTF_FILE

TICKS_PER_SECOND = None
NODE_PROCESS = dict()

####################################################################################################
# Scan for configuration parameters
####################################################################################################

# scan the file to populate the parameters
file = open(DUMP_FILE, "r")

# start iterating through the file
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
        assert(not node in NODE_PROCESS.keys())
        NODE_PROCESS[node] = ids

# flatten the node numbers
index = 0
for key in NODE_PROCESS.keys():
    NODE_PROCESS[index] = NODE_PROCESS[key]
    del NODE_PROCESS[key]
    index = index + 1

# invert the dictionary to create process to node mapping
PROCESS_NODE = dict()
for key in NODE_PROCESS.keys():
    for item in NODE_PROCESS[key]:
        assert(not item in PROCESS_NODE.keys())
        PROCESS_NODE[int(item.strip(" \n\t,"))] = key


assert(TICKS_PER_SECOND != None)
assert(len(NODE_PROCESS.keys()) > 0)
file.close()

####################################################################################################
# Begin Logging Message Events
####################################################################################################

# re-scan the file for the message events
file = open(DUMP_FILE, "r")
LOG_FILE = open(MSG_FILE, "w")

# bookkeeping variables
entries = 0
last_msg_time = 0

# begin the scan through the file
for line in file:

    # process message send events and log them to file
    if (line.find(SEND_MESSAGE_STRING) != -1):
        time = None;
        sender_process = None;
        receiver_process = None;
        length = None;

        # process the time of the event
        tline = line[:line.find(SEND_MESSAGE_STRING)]
        tsplit = tline.split("\t")
        timestamp = tsplit[1]
        timestamp.strip(" \t\n")
        time = float(timestamp) / float(TICKS_PER_SECOND)
        
        # compute the message interarrival time
        if (entries > 0):
            interval = time - last_msg_time
        else:
            interval = 0
        last_msg_time = time

        # process the rest of the fields
        fields = line[line.find(SEND_MESSAGE_STRING) + len(SEND_MESSAGE_STRING):].split(",")
        for field in fields:
            if (field.find(SENDER_TAG) != -1):
                assert(sender_process == None)
                sender_process = int(field[field.find(SENDER_TAG) + len(SENDER_TAG):].strip(", \t\n"))
            elif (field.find(RECEIVER_TAG) != -1):
                assert(receiver_process == None)
                receiver_process = int(field[field.find(RECEIVER_TAG) + len(RECEIVER_TAG):].strip(", \t\n"))
            elif (field.find(LENGTH_TAG) != -1):
                assert(length == None)
                length = int(field[field.find(LENGTH_TAG) + len(LENGTH_TAG):].strip(", \t\n"))

        assert(sender_process != None)
        assert(receiver_process != None)
        assert(length != None)
        assert(time != None)

        assert(sender_process in PROCESS_NODE.keys())
        assert(receiver_process in PROCESS_NODE.keys())

        sender_node = PROCESS_NODE[sender_process]
        receiver_node = PROCESS_NODE[receiver_process]

        entries = entries + 1

        # log the message events to the file
        LOG_FILE.write(str(time) + "," + str(sender_node) + "," + str(sender_process) + "," + str(receiver_node) + "," + str(receiver_process) + "," + str(length) + "," + SEND_FLAG + "," + str(interval) + "\n")

    # process message receive events - TODO: not implemented
    elif (line.find(RECEIVE_MESSAGE_STRING) != -1):
        pass

file.close()

print "[Info] " + str(entries) + " message events written to: " + str(MSG_FILE)

exit(0)
