#!/usr/bin/env bash
####################################################
# launch distcc on slurm allocation
# usage: salloc -N4 distcc.sh make -j
####################################################
srun killall -q distccd
srun distccd --daemon --allow 0.0.0.0/0
nodelist=`scontrol show hostname $SLURM_JOB_NODELIST | xargs`
# hosts="--randomize"
# for n in $nodelist; do
#   hosts="$hosts $n,cpp,lzo"
# done
hosts="--randomize $nodelist" # non-pump mode
export DISTCC_HOSTS="$hosts"
echo "export DISTCC_HOSTS='$hosts'"
exec "$@"
