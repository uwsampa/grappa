#!/bin/bash

export GASNET_SSH_SERVERS='n01,n04'

for id in 1 2 3 4 5
do
#    for list_size_log in 21
    for list_size_log in 19
    do
#	for count in 4
	for count in 1
	do
	    for cores in 10
	    do
		for threads in 128 256 384 512
		do
		    for batch_size in 1 5 10 15 20 25 30 35 40 45 50
		    do
#			for payload_size_log in 0 1 2 3 4 5 6 7 8 9 10
#			for payload_size_log in 0 1 2 3 4 5 6 7 8 9 10
			for payload_size in 0 1 2 4 8 16 32 64 128 256 261
			do
			    CMD1="./listwalk $(( $cores * 2 )):2 -L $list_size_log -n $count -t $threads -I $id -b $batch_size -c $cores --type default"
			    echo Running $CMD1
			    $CMD1
			    for blocktype in inline prefetch block
			    do
				CMD="./listwalk $(( $cores * 2 )):2 -L $list_size_log -n $count -t $threads -I $id -b $batch_size -c $cores --type $blocktype --payload_size $payload_size"
				echo Running $CMD
				$CMD
				echo ---------------------------------------------
				sleep 1
			    done
			done
		    done
		done
	    done
	done
    done
done

