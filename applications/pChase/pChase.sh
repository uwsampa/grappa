#!/bin/bash

pgm=./pChase64_NUMA

b=(8k 16k 24k 32k 48k 64k 96k 128k 192k 256k 384k 512k 768k 1m 1536k 2m 3m 4m 6m 8m 12m 16m)
c=5

date
uname -a
echo
$pgm -o hdr
for page_size in 4k 8k 16k
do
    for threads in 1 2
    do
	for refs in 1 2 4
	do
	    for offset in 0 1
	    do
		for access in random "forward 1"
		do
		    for ((i=0; $i < ${#b[*]}; i++))
		    do
			for ((j=0; $j < $c; j++))
			do 
			    $pgm -p $page_size -t $threads -r $refs -n add $offset -a $access -c ${b[$i]} -s 1.0 -o csv
			done
		    done
	        done
            done
        done
    done
done
echo
cat /proc/cpuinfo
cat /proc/meminfo
date

