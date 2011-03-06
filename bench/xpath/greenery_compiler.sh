#!/usr/bin/env ruby
INPUT= $1
OUTPUT= $2
REPS=$3
TEMPLATE=$(tempfile)
echo "" > $OUTPUT
for i in $(seq 1 $REPS)
do
  echo "void *pcs"$i" = &&label"$i"_BEGIN;" > $OUTPUT
done


echo "goto *pcs1;" > $OUTPUT
echo '#define prefetch_and_switch(x) { __builtin_prefetch(x, 0, 0); pcs` = &&label`_##__LINE__; goto *pcs~; label`_##__LINE__: }' >> $TEMPLATE
sed -e 's/replicate \(.*\)/#define \1 \1`/' -e 's/START/label`_BEGIN:/' $INPUT >> $TEMPLATE

echo $TEMPLATE
for i in $(seq 1 $REPS)
do
  NEXT=$(expr $i % $REPS)
  NEXT=$(expr $NEXT + 1)
  sed -e 's/`/'$i'/g' -e 's/~/'$NEXT'/g' $TEMPLATE | cpp >> $OUTPUT
done
