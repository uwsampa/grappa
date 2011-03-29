#!/bin/sh

CHASES=$1
OUTPUT=$2

cat > $OUTPUT
for i in $(seq 1 32)
do
  make clean && make NTHR=$i
  ./pchase 1 $CHASES > OUTPUT
done
