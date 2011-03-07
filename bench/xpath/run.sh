#!/bin/sh
echo $1 $2 $3
#GOMP_CPU_AFFINITY=0-11:2 hugectl --heap -- numactl --membind=0 -- ./xpath /scratch/20-10000000 1 1 $1 $2 > big-$3-green-$1-$2
GOMP_CPU_AFFINITY=0,12,2,14,4,16,6,18,8,20,10,22 hugectl --heap -- numactl --membind=0 -- ./xpath /scratch/20-10000000 1 1 $1 $2 > big-$3-green-$1-$2
echo Done.
