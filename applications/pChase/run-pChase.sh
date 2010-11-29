#!/bin/bash

dt=`date "+%Y-%m-%d-%H%M"`

./pChase.sh | tee pc-${dt}.csv
