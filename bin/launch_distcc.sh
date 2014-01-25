#!/usr/bin/env bash
####################################################
# launch distcc on slurm allocation
# usage: salloc -N4 distcc.sh make -j
####################################################
# srun killall -q distccd
srun bash -c 'if (( `ps aux | grep distccd | wc -l` == 0 )); then distccd --daemon --allow 0.0.0.0/0; fi'
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
