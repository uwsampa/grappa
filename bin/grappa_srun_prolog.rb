#!/usr/bin/env ruby
interactive = false
if ARGV.size > 1 && ARGV[0] == "--"
  interactive = true
end

## in the future, emit a script to execute
#if __FILE__ == $PROGRAM_NAME
#    foo() # do emit env variables
#end


ENV["GLOG_logtostderr"] = "1"
ENV["GLOG_v"] = "0"
ENV["GASNET_BACKTRACE"] = "1"
ENV["GASNET_FREEZE_SIGNAL"] = "SIGUSR1"
ENV["GASNET_FREEZE_ON_ERROR"] = "1"
ENV["GASNET_FREEZE"] = "0"

# ENV["GASNET_VERBOSEENV"] = "1"

case `hostname`
when /n\d+/ #(sampa)
  ENV["GASNET_PHYSMEM_MAX"] = "1024M"
  # ENV["LD_LIBRARY_PATH"] = "/home/nelson/grappa/tools/built_deps/lib:/sampa/home/nelson/grappa/tools/built_deps/lib/valgrind:/sampa/share/gcc-4.7.2/rtf/lib64:/sampa/share/gcc-4.7.2/src/boost_1_51_0/stage/lib:$LD_LIBRARY_PATH"
end
#ENV["GASNET_PHYSMEM_NOPROBE"] = "1"


#ENV["GASNET_NETWORKDEPTH_PP"] = "1"
#ENV["GASNET_QP_RD_ATOM"] = "1"

#ENV["GASNET_NETWORKDEPTH_PP"] = "96"
#ENV["GASNET_NETWORKDEPTH_TOTAL"] = "1024"
#ENV["GASNET_AM_CREDITS_PP"] = "48"

#ENV["GASNET_FIREHOSE_VERBOSE"] = "1"

#ENV["GASNET_USE_FIREHOSE"] = "0"
#ENV["GASNET_FIREHOSE_M"] = "256M"
#ENV["GASNET_FIREHOSE_MAXVICTIM_M"] = "64K"
#ENV["GASNET_FIREHOSE_MAXREGION_SIZE"] = "64K"
#ENV["GASNET_FIREHOSE_R"] = "16"
#ENV["GASNET_FIREHOSE_MAXVICTIM_R"] = "16"

## these will ensure we use RDMA rather than copying data to a bounce buffer
ENV["GASNET_PACKEDLONG_LIMIT"] = "0"
ENV["GASNET_NONBULKPUT_BOUNCE_LIMIT"] = "0"
ENV["GASNET_PUTINMOVE_LIMIT"] = "0"

#ENV["GASNET_USE_SRQ"] = "2"

#ENV["GASNET_AMRDMA_MAX_PEERS"] = "0"



ENV["CPUPROFILE_FREQUENCY"] = "50"

case `hostname`
when /n\d+/ #(sampa)
    ENV["LD_LIBRARY_PATH"] = "/sampa/home/nelson/grappa/tools/built_deps/lib:/sampa/home/nelson/grappa/tools/built_deps/lib/valgrind:/sampa/share/gcc-4.7.2/rtf/lib64:/sampa/share/gcc-4.7.2/src/boost_1_51_0/stage/lib:#{ENV['LD_LIBRARY_PATH']}"
else
  ENV["LD_LIBRARY_PATH"]="/people/nels707/grappa/tools/built_deps/lib/valgrind:/people/nels707/grappa/tools/built_deps/lib:/pic/people/nels707/boost153-install/lib:/share/apps/mvapich2/1.9b/gcc/4.7.2/lib:/share/apps/gcc/4.7.2/lib:/share/apps/gcc/4.7.2/lib64:/opt/AMDAPP/lib/x86_64:/opt/AMDAPP/lib/x86:#{ENV['LD_LIBRARY_PATH']}"
end

#ENV["VT_VERBOSE"] = "10"
ENV["VT_MAX_FLUSHES"] = "0"
ENV["VT_PFORM_GDIR"] = "."
ENV["VT_PFORM_LDIR"] = "/scratch"
ENV["VT_FILE_UNIQUE"] = "yes"

# ENV["VT_VERBOSE"] = "10"
ENV["VT_MPITRACE"] = "no"

ENV["MV2_USE_LAZY_MEM_UNREGISTER"] = "0"
ENV["MV2_ON_DEMAND_THRESHOLD"] = "3000"

ENV["MV2_DEFAULT_MAX_SEND_WQE"] = "2"
ENV["MV2_DEFAULT_MAX_RECV_WQE"] = "2"
ENV["MV2_MAX_INLINE_SIZE"] = "0"
ENV["MV2_NUM_RDMA_BUFFER"] = "4"

ENV["OMPI_MCA_mpi_leave_pinned"] = "0"

# Clean up any leftover shared memory regions
# for i in `ipcs -m | grep bholt | cut -d" " -f1`; do ipcrm -M $i; done

s = `ipcs -m | grep $USER | cut -d' '  -f1 | xargs -n1 -r ipcrm -M`
s = `rm -f /dev/shm/GrappaLocaleSharedMemory`

if interactive
  exec ARGV[1..-1].join(' ')
end