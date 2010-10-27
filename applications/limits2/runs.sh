#!/bin/bash

for size in 22 
do
	for threads in 1 2 3 4 
	do
		for conc in 12
		do
			#echo "running $size $threads $conc"		
			./linked_list $size $threads $conc > ./data/LL_$size\_$threads\_$conc.out
			python data_rate.py $size $threads $conc
		done
	done
done
