#!/bin/bash

export GLOG_logtostderr=1                                         
export GLOG_v=1
export GASNET_BACKTRACE=1
export GASNET_FREEZE_SIGNAL=SIGUSR1
export GASNET_FREEZE_ON_ERROR=1
export GASNET_FREEZE=0
export GASNET_NETWORKDEPTH_PP=96
export GASNET_NETWORKDEPTH_TOTAL=1024
export GASNET_AMCREDITS_PP=48
export GASNET_PHYSMEM_MAX=1024M
export CPUPROFILE_FREQUENCY=100
export VT_MAX_FLUSHES=0
export VT_PFORM_GDIR=.
export VT_PFORM_LDIR=/scratch

# Clean up any leftover shared memory regions
for i in `ipcs -m | grep bholt | cut -d" " -f1`; do ipcrm -M $i; done
