#!/bin/sh

CHASES=$1
OUTPUT=$2
HYPER=$3
if [ $HYPER = nohyper ]
then
CORES="-6 -5 -4 -3 -2 -1 1 2 3 4 5 6"
PIN="0-11:2"
echo $CORES
elif [ $HYPER = hyper ]
then
CORES="-12 -10 -8 -6 -4 -2 2 4 6 8 10 12"
PIN="0,12,2,14,4,16,6,18,8,20,10,22"
else
 echo "bad value for HYPER: " $HYPER
 exit 1
fi

echo > $OUTPUT
for i in $(seq 1 32)
do
  make clean && make NTHR=$i pchase
  for c in $CORES
  do
  echo $i $c
  GOMP_CPU_AFFINITY=$PIN hugectl --heap -- numactl --membind=0 -- ./pchase $c $CHASES $i >> $OUTPUT
  done
done
