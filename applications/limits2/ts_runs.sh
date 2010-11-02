#!/bin/bash

for size in 22 
do
	for threads in 1 2 3 4
	do
		ts="0x1"
	if [ $threads -eq 2 ]
	then
		ts="0x5"
	fi
	if [ $threads -eq 3 ]
	then
		ts="0x15"
	fi
	if [ $threads -eq 4 ]
	then
		ts="0x55"
	fi
		for conc in 1 2 3 4 5 6 7 8 9 10 11 12
		do
			#echo "running $size $threads $conc"		
			taskset $ts ./linked_list $size $threads $conc > ./ts_data2/LL_$size\_$threads\_$conc.out
			python data_rate.py $size $threads $conc
		done
	done
done
