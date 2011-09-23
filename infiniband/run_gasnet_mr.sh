#!/bin/bash

for count in 10000000
  do
  for repeat in 1 2 3 4 5
    do
    for method in ib mpi
      do
      for single_dest in "" -0
	do
	for commtype in rdma_write rdma_read messages
	  do
	  for sockets in 1 2
	    do
	    for cores in 1 2 3 4
	      do
	      for threads in 1 2 3 4 5 6 7
		do
		if [ $(( $sockets * $cores * $threads )) -le 7 ]
		    then 
		    echo RUNNING salloc --exclusive -N 2 -B $sockets:$cores:1 --cpu_bind=none --mem_bind=none \
			-- mpirun ./gasnet_mr_$method --$commtype -n $count -c $cores -k $sockets -t $threads $single_dest
		    time salloc --exclusive -N 2 -B $sockets:$cores:1 --cpu_bind=none --mem_bind=none \
			-- mpirun ./gasnet_mr_$method --$commtype -n $count -c $cores -k $sockets -t $threads $single_dest
		fi
	      done
	    done
	  done
	done
      done
    done
  done
done
