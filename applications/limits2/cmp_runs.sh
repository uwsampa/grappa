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
	#echo "$threads $ts"
	 
		for conc in 2 3 4 5 6 7 8 9 10 11 12
		do
			#echo "running $size $threads $conc"		
			python ../../exper_runner/loop_runexp.py "--follow-all --aggregate-results -e mem_load_retired:llc_miss" "taskset $ts ./linked_list $size $threads $conc -i" > ./cmp_data/LL_$size\_$threads\_$conc.out
			python compare.py $size $threads $conc total_requests "MEM_LOAD_RETIRED:LLC_MISS"
		done
	done
done
