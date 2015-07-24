## set up Google logging defaults
export GLOG_logtostderr="1"
export GLOG_v="1"

## set Google profiler sample rate
export CPUPROFILE_FREQUENCY="50"

## set VampirTrace options
#export VT_VERBOSE="10"
export VT_MAX_FLUSHES="0"
export VT_PFORM_GDIR="."
export VT_PFORM_LDIR="/scratch"
export VT_FILE_UNIQUE="yes"
export VT_MPITRACE="no"
export VT_UNIFY="no"

## set MVAPICH2 options to avoid keeping around malloced memory
## (and some performance tweaks which may be irrelevant)
export MV2_USE_LAZY_MEM_UNREGISTER="0"
export MV2_HOMOGENEOUS_CLUSTER="1"

export MV2_USE_RDMA_FAST_PATH="0"

export MV2_SRQ_MAX_SIZE="8192"
#export MV2_USE_XRC="1" # doesn't seem to work with 1.9b on pal

#export MV2_USE_MCAST="1" # doesn't always work on pal

## set MVAPICH2 options to avoid keeping around malloced memory
export OMPI_MCA_mpi_leave_pinned="0"
export OMPI_MCA_mpi_yield_when_idle="0"

# in case $USER isn't set
USER=${USER-$(whoami)}
