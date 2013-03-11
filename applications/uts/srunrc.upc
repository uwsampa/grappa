#!/bin/bash
echo export GASNET_BACKTRACE=1
echo export GASNET_FREEZE_ON_ERROR=1
#echo export GASNET_FREEZE=0
#echo export GASNET_PHYSMEM_MAX=1500M

# from upcrun
echo export GASNET_FCA_ENABLE_REDUCE='1'
echo export GASNET_FCA_ENABLE_BCAST='1'
echo export GASNET_FCA_ENABLE_BARRIER='1'
#echo export GASNET_MAX_SEGSIZE='4GB'
#echo export UPC_SHARED_HEAP_SIZE='1024MB'
#echo export UPC_SHARED_HEAP_SIZE_MAX='1GB'
