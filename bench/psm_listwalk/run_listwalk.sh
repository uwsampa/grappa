#!/bin/bash

export GASNET_SSH_SERVERS='n01,n04'

for id in 1 2 3 4 5
do
    for list_size_log in 21
    do
	for count in 16
	do
#	    for cores in 1 2 3 4 5 6 7 8 9 10
	    for cores in 1 2 4 6 8 10
#	    for cores in 1 10
	    do
		for threads in 1 4 8 16 64 128 256 384 512 768
#		for threads in 1 64 128 256 384 512
		do
		    for batch_size in 1 2 3 4 5 10 15 20 25 30 35 40 45 50
#		    for batch_size in 
		    do
			./listwalk $(( $cores * 2 )):2 -L $list_size_log -n $count -t $threads -I $id -b $batch_size -c $cores
		    done
		done
	    done
	done
    done
done

