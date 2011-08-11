#!/bin/sh
# ghetto hack - should be fixed. place absolute path of this directory in LOCATION
LOCATION=$(dirname $0)
INPUT=$1
OUTPUT=$2
REPS=$3
TEMPLATE=$(tempfile)

SCHED_INST_NAME="_jsched"
THREADS_INST_PREFIX="_jthread"

OUTPUTTEMP=$(tempfile)

echo "" > $OUTPUTTEMP
echo "jthr_sched $SCHED_INST_NAME;" >> $OUTPUTTEMP #TODO put in template
echo "jthr_sched_init(&$SCHED_INST_NAME);" >> $OUTPUTTEMP
for i in $(seq 1 $REPS)
do
  #echo "void *pcs"$i" = &&label"$i"_BEGIN;" >> $OUTPUTTEMP
  echo "jthr_thread $THREADS_INST_PREFIX"$i";" >> $OUTPUTTEMP
  echo "jthr_thread_init(&$THREADS_INST_PREFIX"$i");" >> $OUTPUTTEMP
  echo "jthr_enqueue(&$SCHED_INST_NAME, &$THREADS_INST_PREFIX"$i", &&label"$i"_BEGIN);" >> $OUTPUTTEMP
done


echo "int active = "$REPS";" >> $OUTPUTTEMP

cat $LOCATION/preamble.greenery > $TEMPLATE
sed -e 's/replicate \(.*\)/#define \1 \1`/' -e 's/START/START\nlabel`_BEGIN:/'  $INPUT >> $TEMPLATE
cat $LOCATION/postamble.greenery >> $TEMPLATE

DECLTEMP=$(tempfile)
CODETEMP=$(tempfile)
sed -e '/DECLARATIONS/,/START/d' $TEMPLATE > $CODETEMP
sed -n -e '/DECLARATIONS/d' -e '/START/q' -e p $TEMPLATE > $DECLTEMP
echo $TEMPLATE
echo $DECLTEMP
echo $CODETEMP
for i in $(seq 1 $REPS)
do
  NEXT=$(expr $i % $REPS)
  NEXT=$(expr $NEXT + 1)
  sed -e 's/`/'$i'/g' -e 's/~/'$NEXT'/g' $DECLTEMP | cpp | sed -e '/^#/d' >> $OUTPUTTEMP
done
echo "goto *(jthr_dequeue(&$SCHED_INST_NAME));" >> $OUTPUTTEMP

for i in $(seq 1 $REPS)
do
  NEXT=$(expr $i % $REPS)
  NEXT=$(expr $NEXT + 1)
  sed -e 's/`/'$i'/g' -e 's/~/'$NEXT'/g' $CODETEMP | cpp | sed -e '/^#/d' >> $OUTPUTTEMP
done

echo "label_END:" >> $OUTPUTTEMP


#echo '#include "jumpthreads.h"'  > $OUTPUT
cat $OUTPUTTEMP > $OUTPUT
