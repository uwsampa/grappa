#!/bin/sh

# Copies all ga_*.m4 files from GA's m4 directory and reimages them to be
# tascel's.

for f in ../../m4/ga_*.m4
do
    base_name=`basename $f`
    new_name=`echo $base_name | sed s/ga_/tascel_/`
    echo "cp $f $new_name"
    cp $f $new_name
    echo "sed -i 's/ga_/tascel_/g' $new_name"
    sed -i 's/ga_/tascel_/g' $new_name
    echo "sed -i 's/GA_/TASCEL_/g' $new_name"
    sed -i 's/GA_/TASCEL_/g' $new_name
done
