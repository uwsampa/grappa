#!/usr/bin/env ruby
interactive = false
if ARGV.size > 1 && ARGV[0] == "--"
  interactive = true
end

## set up Google logging defaults
ENV["GLOG_logtostderr"] = "1"
ENV["GLOG_v"] = "1"

## set Google profiler sample rate
ENV["CPUPROFILE_FREQUENCY"] = "50"

## set VampirTrace options
#ENV["VT_VERBOSE"] = "10"
ENV["VT_MAX_FLUSHES"] = "0"
ENV["VT_PFORM_GDIR"] = "."
ENV["VT_PFORM_LDIR"] = "/scratch"
ENV["VT_FILE_UNIQUE"] = "yes"
ENV["VT_MPITRACE"] = "no"
ENV["VT_UNIFY"] = "no"

## set MVAPICH2 options to avoid keeping around malloced memory
## (and some performance tweaks which may be irrelevant)
ENV["MV2_USE_LAZY_MEM_UNREGISTER"] = "0"
ENV["MV2_HOMOGENEOUS_CLUSTER"] = "1"

ENV["MV2_USE_RDMA_FAST_PATH"] = "0"

ENV["MV2_SRQ_MAX_SIZE"] = "8192"
#ENV["MV2_USE_XRC"] = "1" # doesn't seem to work with 1.9b on pal

#ENV["MV2_USE_MCAST"] = "1" # doesn't always work on pal

## set MVAPICH2 options to avoid keeping around malloced memory
ENV["OMPI_MCA_mpi_leave_pinned"] = "0"
ENV["OMPI_MCA_mpi_yield_when_idle"] = "0"

## Clean up any leftover shared memory regions
s = `ipcs -m | grep $USER | awk '{print $2}' | xargs -n1 -r ipcrm -m`
s = `rm -f /dev/shm/GrappaLocaleSharedMemory`

if interactive
  exec ARGV[1..-1].join(' ')
end
