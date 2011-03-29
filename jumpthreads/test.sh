#!/bin/sh

CHASES=$1
OUTPUT=$2

echo > $OUTPUT
for i in $(seq 1 1)
do
  make clean && make NTHR=$i
#  for c in -6 -5 -4 -3 -2 -1 1 2 3 4 5 6
  for c in -12 -10 -8 -6 -4 -2 2 4 6 8 10 12
  do
#  GOMP_CPU_AFFINITY=0-11:2 hugectl --heap -- numactl --membind=0 -- ./pchase $c $CHASES $i >> $OUTPUT
  GOMP_CPU_AFFINITY=0,12,2,14,4,16,6,18,8,20,10,22 hugectl --heap -- numactl --membind=0 -- ./pchase $c $CHASES $i >> $OUTPUT
  done
done
