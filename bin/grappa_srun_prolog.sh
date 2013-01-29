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
# Clean up any leftover shared memory regions
for i in `ipcs -m | grep bholt | cut -d" " -f1`; do ipcrm -M $i; done
