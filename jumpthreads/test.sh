#!/bin/sh

CHASES=$1
OUTPUT=$2
HYPER=$3
CORES="-6 -5 -4 -3 -2 -1 1 2 3 4 5 6"
if [ $HYPER = NOHYPER ]
then
PIN="0-11:2"
elif [ $HYPER = HYPER ]
then
PIN="0,12,2,14,4,16,6,18,8,20,10,22"
else
 echo "bad value for HYPER: " $HYPER
 exit 1
fi

echo > $OUTPUT
for p in 0 1
do
for i in $(seq 1 32)
do
  make clean && make NTHR=$i HYP=$HYPER PF=$p pchase 
  for c in $CORES
  do
  echo $i $c
  nice -n -20 hugectl --heap -- numactl --membind=0 -- ./pchase $c $CHASES $i$HYPER >> $OUTPUT
  done
done
done
