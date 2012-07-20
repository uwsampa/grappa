#
# Makefile for SoftXMT V0
#
# This provides automatic header dependence discovery, 
# as well as an easiery way to run tests
#
# Filenames:
#  - C++ header files are named <name>.hpp;
#    C header files are named <name>.h
#  - Each .cpp file without a main() can include tests and be used to
#    make a .test. These are ususally named <name>_tests.cpp
#  - Executables are named <name>.exe.
#  - Test executables are named <name>.test. 
#  - Tests are run using MPI.
#
# Other notes: 
#  - $LD is actually a C++ compiler, probably mpic++. Individual files
#    are compiled with $CXX; linking is done with $LD.
#  - default directories and flags should be set in a common
#    include.mk, but actual library includes should be set here.
#
# Steps for use
# 1. Add library dependences to $(LIBRARIES) below.
# 2. Add compiler flags to $(CFLAGS) below.
# 3. Add object file dependences below.
# 4. Set a default target below if you want.
# 4. To build the default target, say "make".
#    To build an executable or test, say "make TARGET=<name>.exe" or "make TARGET=name.test".
#    To build and run a test, say "make TARGET=<name>.test test". 
#    To build and run an executable, say "make TARGET=<name>.test run". 
#

# include common variable definitions
include $(SOFTXMT_HOME)/include.mk

#
# Define DEBUG to build debug versions of binaries/tests
# This may be done at the command line: "make TARGET=foo.test DEBUG=1 test"
#
#DEBUG:=1

#
# name of host on which to run tests
# (unused with srun)
#
HOST:=n01,n04

#
# number of mpi processes
#
NPROC:=2

# name of main executable to build and/or run
TARGET:=

# default rule: just build
$(TARGET):

# library build rule
ifdef DYNAMIC_LIB
LIBRARY=libSoftXMT.so
else
LIBRARY=libSoftXMT.a
endif
lib: $(LIBRARY)

#
# object file dependences
#
Demo.o Demo.exe: $(LIBRARY)

Communicator_tests.o Communicator_tests.test: Communicator.o

Aggregator_tests.o Aggregator_tests.test: Communicator.o Aggregator.o Timestamp.o
MutableHeap_tests.o MutableHeap_tests.test: 


Delegate_tests.o Delegate_tests.test: $(LIBRARY)
Addressing_tests.o Addressing_tests.test: $(LIBRARY)
GlobalMemoryChunk_tests.o GlobalMemoryChunk_tests.test: $(LIBRARY)
GlobalMemory_tests.o GlobalMemory_tests.test: $(LIBRARY)
GlobalAllocator_tests.o GlobalAllocator_tests.test: $(LIBRARY)
Allocator_tests.o Allocator_tests.test: Allocator.o

QueueDebug_tests.o QueueDebug_tests.exe: ../tools/stl-debug-allocator/archive/Source/allocator.o

Collective_tests.o Collective_tests.test: $(LIBRARY)

#thread_tests.test: thread.o stack.o coro.o
thread2_tests.o thread2_tests.test: $(LIBRARY)
thread3_tests.o thread3_tests.test: $(LIBRARY)
tasks/Thread_tests.o tasks/Thread_tests.test: tasks/Thread.o coro.o stack.o tasks/BasicScheduler.o Timestamp.o

Cache_tests.o Cache_tests.test: $(LIBRARY)
RateMeasure_tests.o RateMeasure_tests.test: $(LIBRARY)
CacheWrapped_tests.o CacheWrapped_tests.test: $(LIBRARY)

ForkJoin_tests.o ForkJoin_tests.test: $(LIBRARY)
ForkJoinStress_tests.o ForkJoinStress_tests.test: $(LIBRARY)

Tasking_tests.o Tasking_tests.test: $(LIBRARY)
Stealing_tests.o Stealing_tests.test: $(LIBRARY)
ForkJoin_tests.o ForkJoin_tests.test: $(LIBRARY)
Future_tests.o Future_tests.test: $(LIBRARY)
ParallelLoop_tests.o ParallelLoop_tests.test: $(LIBRARY)
ParallelLoopPerformance_tests.o ParallelLoopPerformance_tests.test: $(LIBRARY)
RemotePrivateTasks_tests.o RemotePrivateTasks_tests.test: $(LIBRARY)

AsyncParallelFor_tests.o AsyncParallelFor_tests.test: AsyncParallelFor.o
AsyncParallelFor_Local_tests.o AsyncParallelFor_Local_tests.test: $(LIBRARY)
AsyncParallelFor_Global_tests.o AsyncParallelFor_Global_tests.test: $(LIBRARY)
AsyncParallelFor_Rate.o AsyncParallelFor_Rate.exe: $(LIBRARY)

NetworkPerformance_tests.test NetworkPerformance_tests.o: $(LIBRARY)
#
# environment variables for run
# remember to include environment's $LD_LIBRARY_PATH, since MPI libs may be there
#
ENV_VARIABLES+= GLOG_logtostderr=1
#ENV_VARIABLES+= GLOG_vmodule=Cache*=5,Incoh*=5,Aggreg*=5
#ENV_VARIABLES+= GLOG_vmodule=Cache*=5,Incoh*=5
ENV_VARIABLES+= GLOG_v=1
#ENV_VARIABLES+= OMPI_MCA_btl_sm_use_knem=0
ENV_VARIABLES+= LD_LIBRARY_PATH="\$$LD_LIBRARY_PATH:$(LD_LIBRARY_PATH)"

# work around 1GB huge page bug in gasnet by limiting size of memory 
#ENV_VARIABLES+= GASNET_PHYSMEM_MAX=217M # appears to be minimum
ENV_VARIABLES+= GASNET_PHYSMEM_MAX=1024M
#ENV_VARIABLES+= GASNET_PHYSMEM_NOPROBE=0

# disabling this by setting it to 0 may reduce delays
# but so may enabling it. best leave this commented out.
#ENV_VARIABLES+= GASNET_AMRDMA_LIMIT=0

# 0 is a safe value, but more (3?) may provide a performance boost
# I'm not seeing one, though. Best leave commented out.
#ENV_VARIABLES+= GASNET_NUM_QPS=3

# adjusting these may do something....
# but I didn't see much difference
#ENV_VARIABLES+= GASNET_PSHMNET_QUEUE_DEPTH=8
#ENV_VARIABLES+= GASNET_PSHMNET_QUEUE_MEMORY=1MB



# gasnet debugging
ENV_VARIABLES+= GASNET_BACKTRACE=1
ENV_VARIABLES+= GASNET_FREEZE_SIGNAL=SIGUSR1
ENV_VARIABLES+= GASNET_FREEZE_ON_ERROR=1
ENV_VARIABLES+= GASNET_FREEZE=0
ENV_VARIABLES+= GASNET_NETWORKDEPTH_PP=96
ENV_VARIABLES+= GASNET_NETWORKDEPTH_TOTAL=1024
ENV_VARIABLES+= GASNET_AMCREDITS_PP=48



#
# Heap Leak Checker
# we must set environment variable HEAPCHECK=local to use annotations in code
#
ifdef HEAPCHECK
ENV_VARIABLES+= HEAPCHECK=local
ENV_VARIABLES+= HEAP_CHECK_DUMP_DIRECTORY=.
CFLAGS+= -DHEAPCHECK
LIBRARIES+= -ltcmalloc
endif

ifdef GOOGLE_PROFILER
#NOTE: no output is generated by default unless you call the API or set this variable:
#ENV_VARIABLES+= CPUPROFILE="\$$SLURM_JOB_NAME.\$$SLURM_JOB_ID.\$$SLURM_PROCID.prof"
# how often should we sample? (interrupts/sec)
ENV_VARIABLES+= CPUPROFILE_FREQUENCY=100
CFLAGS+= -fno-omit-frame-pointer -DGOOGLE_PROFILER
LIBS+= -lprofiler
endif

# activate this for sampled traces
ifdef VTRACE_SAMPLED
#ENV_VARIABLES+= VT_VERBOSE=10
#ENV_VARIABLES+= VT_BUFFER_SIZE=512M
ENV_VARIABLES+= VT_MAX_FLUSHES=0
ENV_VARIABLES+= VT_PFORM_GDIR=.
ENV_VARIABLES+= VT_PFORM_LDIR=/scratch
VAMPIRTRACE_COMMON_ARGS= -vt:inst manual
CXX=$(VAMPIRTRACE)/bin/vtcxx -vt:cxx g++ $(VAMPIRTRACE_COMMON_ARGS)
CC=$(VAMPIRTRACE)/bin/vtcc -vt:cc gcc $(VAMPIRTRACE_COMMON_ARGS)
LD=$(VAMPIRTRACE)/bin/vtcxx -vt:cxx mpicxx $(VAMPIRTRACE_COMMON_ARGS)
CFLAGS+= -DVTRACE -DVTRACE_SAMPLED
endif


ifdef STL_DEBUG_ALLOCATOR
CFLAGS+= -DSTL_DEBUG_ALLOCATOR
../tools/stl-debug-allocator/archive/Source/allocator.o: CFLAGS+= -DUNIX
LIBRARY_CONTENTS+= ../tools/stl-debug-allocator/archive/Source/allocator.o
Aggregator_tests.test: ../tools/stl-debug-allocator/archive/Source/allocator.o
endif

ifdef CORO_PROTECT_UNUSED_STACK
CFLAGS+= -DCORO_PROTECT_UNUSED_STACK
endif

ifdef DYNAMIC_LIB
CFLAGS+= -fPIC -DDYNAMIC_LIB
endif

#
# Build SoftXMT static library
#
LIBRARY_CONTENTS+= SoftXMT.o GlobalAllocator.o GlobalMemory.o GlobalMemoryChunk.o Communicator.o Aggregator.o Timestamp.o Allocator.o Cache.o Delegate.o Collective.o ForkJoin.o tasks/cbarrier.o tasks/TaskingScheduler.o tasks/Task.o tasks/Thread.o stack.o coro.o StateTimer.o PerformanceTools.o Future.o CurrentThread.o ProfilerGroups.o AsyncParallelFor.o GlobalTaskJoiner.o IncoherentAcquirer.o IncoherentReleaser.o

libSoftXMT.a: $(LIBRARY_CONTENTS)
	$($(MPITYPE)_AR) rv $@ $^

ifdef DYNAMIC_LIB
libSoftXMT.so: $(LIBRARY_CONTENTS)
	$($(MPITYPE)_LD) -shared -Wl,-soname,$@.1 -o $@.1.0 $^
	ln -sf $@.1.0 $@
	ln -sf $@.1.0 $@.1
endif

#
# run a binary
#
mpi_run: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP) $($(MPITYPE)_EPILOG_TEMP) $($(MPITYPE)_BATCH_TEMP)
	-killall $<
	$(ENV_VARIABLES) \
	$($(MPITYPE)_RUN) ./$< $(GARGS)

run: $(TARGET)
	-killall $<
	$(ENV_VARIABLES) \
	./$< $(ARGS)

#
# run tests for a component
#
mpi_test: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP) $($(MPITYPE)_EPILOG_TEMP) $($(MPITYPE)_BATCH_TEMP)
	$(ENV_VARIABLES) \
	$($(MPITYPE)_RUN) ./$< --log_level=test_suite --run_test=$(TARGET:.test=) $(GARGS)

test: $(TARGET)
	-killall $<
	$(ENV_VARIABLES) \
	./$< --log_level=test_suite --run_test=$(TARGET:.test=) -- $(ARGS)


# TODO: fixme so I'm not so dumb
cleanup_shm:
	echo "State before cleanup:"
	for i in `sinfo -h -p grappa -o '%n'`; do ssh $$i bash -c "echo;hostname; ipcs -m; grep Huge /proc/meminfo"; done
	echo "Cleaning up...."
	for i in `sinfo -h -p grappa -o '%n'`; do ssh $$i bash -c "ipcs -m | grep $(USER) | cut -d' ' -f1 | xargs -n1 -r ipcrm -M"; done
	echo "State after cleanup:"
	for i in `sinfo -h -p grappa -o '%n'`; do ssh $$i bash -c "echo; hostname; ipcs -m; grep Huge /proc/meminfo"; done


# build a specific test
# include Boost test automagic main() generation
# add an additional rule if you need to include multiple components
%_tests.o: CXXFLAGS+= -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=$@
%_tests.test: LIBRARIES+= -lboost_unit_test_framework
%_tests.test: %_tests.o
	$($(MPITYPE)_LD) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBRARIES) -o $@ $(LIBS)

# disassemble test
%_tests.dis: %_tests.test
	objdump -S $< > $@

#
# What libraries do we need?
#
LIBRARIES+= -lgflags -lglog -lrt $(GASNET_LIBS)

#
# set compiler flags
#
CFLAGS+= -Winline
CFLAGS+= $(GASNET_FLAGS)
CFLAGS+= -D_GNU_SOURCE

# not sure if this is a good idea
CFLAGS+= -Wno-inline
#CFLAGS+= -Wall -Wextra
CFLAGS+= -I. -I./tasks

#
# set debug flags
#
ifdef DEBUG
CFLAGS+= -O1 -g -DDEBUG
else
ifdef PROFILE
CFLAGS+= -O3 -DNDEBUG -pg
else
CFLAGS+= -O3 -DNDEBUG
endif #PROFILE
endif #DEBUG


#
# set Tau performance profiling instrumentation flags
#
ifdef TAU_INST
TAUDIR=/sampa/share/tau-perf/tau
include $(TAUDIR)/x86_64/lib/Makefile.tau-mpi-pdt-trace
#TAU_COMP_OPTIONS="-optCompInst -optTauSelectFile=tau.conf"
TAU_COMP_OPTIONS="-optCompInst"
CC:=$(TAU_COMPILER) $(TAU_COMP_OPTIONS) $(CC)
CXX:=$(TAU_COMPILER) $(TAU_COMP_OPTIONS) $(CXX)
LD:=$(TAU_COMPILER) $(TAU_COMP_OPTIONS) $(LD)
endif

#
# set flags to use Tau API
#

ifdef TAU_API
TAUDIR=/sampa/share/tau-perf/tau
include $(TAUDIR)/include/Makefile
CFLAGS+= $(TAU_INCLUDE) 

CXX= $(TAU_CXX) 
CC= $(TAU_CC)
LD= $(TAU_MPICC)
CFLAGS+= $(TAU_DEFS) -DGRAPPA_TRACE -Wno-write-strings
LIBS= $(TAU_LIBS)
LDFLAGS+= $(USER_OPT)
MY_TAU_RUN=$(TAUDIR)/x86_64/bin/tau_load.sh
ENV_VARIABLES+= TAU_METRICS=LINUX_TIMERS
endif

# activate this for fine-grained traces (doesn't work properly yet)
ifdef VTRACE_FULL
#ENV_VARIABLES+= VT_VERBOSE=10
ENV_VARIABLES+= VT_BUFFER_SIZE=512M
ENV_VARIABLES+= VT_MAX_FLUSHES=100
ENV_VARIABLES+= VT_PFORM_GDIR=.
ENV_VARIABLES+= VT_PFORM_LDIR=/scratch
VAMPIRTRACE_COMMON_ARGS= -vt:inst manual
CXX=$(VAMPIRTRACE)/bin/vtcxx -vt:cxx g++ $(VAMPIRTRACE_COMMON_ARGS)
CC=$(VAMPIRTRACE)/bin/vtcc -vt:cc gcc $(VAMPIRTRACE_COMMON_ARGS)
LD=$(VAMPIRTRACE)/bin/vtcxx -vt:cxx mpicxx $(VAMPIRTRACE_COMMON_ARGS)
CFLAGS+= -DVTRACE -DVTRACE_FULL
endif


#
# set LD_LIBRARY_PATH
#
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib:$(SOFTXMT_HOME)/system

# build an executable
# add an additional rule if you need to include multiple components
%.exe: %.o 
	$(ENV_VARIABLES) \
	$($(MPITYPE)_LD) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBRARIES) -o $@ $(LIBS)

# disassemble executable
%.dis: %.exe
	objdump -S $< > $@

# build documentation
doc:: 
	doxygen Doxyfile


#
# autodependency stuff
#

# pull in dependency info for *existing* .o files
OBJECTS:=$(wildcard *.o)
-include $(OBJECTS:.o=.d)

TESTS:=$(wildcard *.test)
-include $(TESTS:.test=.d)

# build an object
# includes autodependence stuff
%.o: %.cpp
	$(ENV_VARIABLES) \
	$($(MPITYPE)_CXX) -MMD -MF $*.d.tmp -c $(CFLAGS) $(CXXFLAGS) $< -o $*.o
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

# build a pre-processed
%.preproc: %.cpp
	$(ENV_VARIABLES) \
	$($(MPITYPE)_CXX) -E -MMD -MF $*.d.tmp $(CFLAGS) $(CXXFLAGS) $< -o $*.preproc

# build a precompiled header
# includes autodependence stuff
%.pch: %.hpp
	$(ENV_VARIABLES) \
	$($(MPITYPE)_CXX) -MMD -MF $*.d.tmp -c $(CFLAGS) $(CXXFLAGS) $< -o $*.pch
	@sed -e 's|.*:|$*.pch:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

#
# Temporary special handling for thread library
#

coro.o: coro.c
	$(ENV_VARIABLES) \
	$($(MPITYPE)_CXX) -MMD -MF $*.d.tmp -c $(CFLAGS) $< -o $*.o
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

stack.o: stack.S
	$(ENV_VARIABLES) \
	$($(MPITYPE)_CC) -c $(CFLAGS) $< -o $*.o


CLEAN_FILES+= *.exe *.o *.pch *.d *.test *.preproc *.dis libSoftXMT.a libSoftXMT.so* ./tasks/*.o ./tasks/*.preproc $(LIBRARY_CONTENTS) $($(MPITYPE)_CLEAN_FILES)
clean:
	rm -f $(CLEAN_FILES)

.PHONY: run test clean
