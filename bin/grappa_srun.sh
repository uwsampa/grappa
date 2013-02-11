#!/bin/bash

# directory of current file
DIR=$PWD/`dirname $BASH_SOURCE[0]`

# pull out args up to separator ('--') to parse with `getopt`
toparse=()
while [ $# -gt 0 ]; do
  case $1 in
    (--) echo $1; shift; break;;
    (*) toparse+=($1); shift;;
  esac
done
remaining=$* # save rest of args to pass to `srun` as the actual command
opts=$(getopt -o n:c:ht: -l help,nodes:,cores-per-node:,test: -- "$toparse")
set -- $opts # put $opts back into bash builtin args (accessible by $@, $1, shift, etc)

function print_usage {
  echo "Script to run Grappa applications."
  echo "Usage: $BASH_SOURCE [options] -- <executable> [executable options]"
  echo "  -n, --nodes           Number of nodes to run the Grappa job with"
  echo "  -c, --cores-per-node  Number of cores per node"
  echo "  -h, --help            Print this help message"
  echo "  -t, --test            Run boost unit test program with given name (e.g. Aggregator_tests)"
}

while [ $# -gt 0 ]; do
# for i in ${opts[@]}; do
  # case $i in
  case $1 in
    (-n | --nodes) NODES=$2; shift;;
    (-c | --cores-per-node) CORES_PER_NODE=$2; shift;;
    (-h | --help) print_usage; exit;;
    (-t | --test) TEST_ARGS="system/$2.test --log_level=test_suite --report_level=confirm --run_test=$2"; shift;;
    (--) shift; break;;
    (-*) echo "$0: error - unrecognized option $1" 1>&2; exit 1;;
    (*)  break;;
  esac
  shift
done

# defaults
NODES=${NODES-2}
CORES_PER_NODE=${CORES_PER_NODE-1}

GRAPPA_SRUN_FLAGS="--cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit --task-prolog=$DIR/grappa_srun_prolog.sh --task-epilog=$DIR/grappa_srun_epilog.sh"
SAMPA_FLAGS="--resv-ports --partition=grappa"

source "$DIR/grappa_srun_prolog.sh"

# echo srun --nodes=$NODES --ntasks-per-node=$CORES_PER_NODE $GRAPPA_SRUN_FLAGS $SAMPA_FLAGS -- $remaining
eval srun --nodes=$NODES --ntasks-per-node=$CORES_PER_NODE $GRAPPA_SRUN_FLAGS $SAMPA_FLAGS -- $TEST_ARGS $remaining
