#! /bin/bash

for i in `seq 256 -32 4 `; do
    for j in 1000000 10000000 100000000 1000000000 10000000000; do
	mpirun -np $i ./psort $j s o </dev/null; 
	mpirun -np $i ./psort $j m o </dev/null; 
	mpirun -np $i ./psort $j m f </dev/null; 
	mpirun -np $i ./psort $j m t </dev/null; 
    done; 
done

for i in 254 192 128; do
	mpirun -np $i ./psort 100000000000 m o
	mpirun -np $i ./psort 100000000000 m f
done
