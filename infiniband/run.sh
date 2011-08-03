#!/bin/bash

for outstanding in 1000 100 10 5 1
do
    for payload_log in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
    do
	mpirun -mca btl ^openib --bind-to-core -np 2 ./small_messages --payload_size_log=$payload_log --outstanding=$outstanding
    done
done


