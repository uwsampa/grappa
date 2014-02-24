#!/usr/bin/env bash
####################################################
# launch distcc on slurm allocation
# assumes that distcc is already running on those nodes
# usage: salloc -N4 distcc.sh make -j
####################################################
nodelist=`scontrol show hostname $SLURM_JOB_NODELIST | xargs`
# hosts="--randomize"
# for n in $nodelist; do
#   hosts="$hosts $n,cpp,lzo"
# done
hosts="--randomize $nodelist" # non-pump mode
export DISTCC_HOSTS="$hosts"
export PS1="(distcc) $PS1"
echo "export DISTCC_HOSTS='$hosts'"
exec "$@"
