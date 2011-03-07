#!/bin/sh
INPUT=$1
OUTPUT=$2
REPS=$3
TEMPLATE=$(tempfile)
echo "" > $OUTPUT
for i in $(seq 1 $REPS)
do
  echo "void *pcs"$i" = &&label"$i"_BEGIN;" >> $OUTPUT
done

echo "int active = "$REPS";" >> $OUTPUT

cat preamble.greenery > $TEMPLATE
sed -e 's/replicate \(.*\)/#define \1 \1`/' -e 's/START/START\nlabel`_BEGIN:/'  $INPUT >> $TEMPLATE
cat postamble.greenery >> $TEMPLATE

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
  sed -e 's/`/'$i'/g' -e 's/~/'$NEXT'/g' $DECLTEMP | cpp | sed -e '/^#/d' >> $OUTPUT
done
echo "goto *pcs1;" >> $OUTPUT

for i in $(seq 1 $REPS)
do
  NEXT=$(expr $i % $REPS)
  NEXT=$(expr $NEXT + 1)
  sed -e 's/`/'$i'/g' -e 's/~/'$NEXT'/g' $CODETEMP | cpp | sed -e '/^#/d' >> $OUTPUT
done

echo "label_END:" >> $OUTPUT
