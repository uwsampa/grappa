#!/bin/bash

for outstanding in 1000 100 10 5 1
do
    for batch_size in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
    do
	for payload_log in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30
	do
	    mpirun -mca btl ^openib --bind-to-core -np 2 ./small_messages \
		--payload_size_log=$payload_log \
		--outstanding=$outstanding \
		--batch_size=$batch_size \
		--rdma_write
	    mpirun -mca btl ^openib --bind-to-core -np 2 ./small_messages \
		--payload_size_log=$payload_log \
		--outstanding=$outstanding \
		--batch_size=$batch_size \
		--rdma_read
    done
done


