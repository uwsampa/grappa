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

lib: libSoftXMT.a

#
# object file dependences
#
Demo.exe: SoftXMT.o GlobalMemory.o Communicator.o Aggregator.o Timestamp.o thread.o coro.o stack.o

Communicator_tests.test:

Aggregator_tests.test: Communicator.o Aggregator.o Timestamp.o
MutableHeap.test:


Delegate_tests.test: Delegate.o SoftXMT.o GlobalMemory.o  Timestamp.o Communicator.o Aggregator.o thread.o coro.o stack.o 
Addressing_tests.test: SoftXMT.o GlobalMemory.o GlobalAllocator.o Timestamp.o Allocator.o Communicator.o Aggregator.o thread.o coro.o stack.o 
GlobalMemoryChunk_tests.test: Delegate.o SoftXMT.o GlobalMemory.o Timestamp.o Communicator.o Aggregator.o thread.o coro.o stack.o 
GlobalAllocator_tests.test: GlobalAllocator.o Allocator.o Delegate.o SoftXMT.o Timestamp.o GlobalMemory.o  Communicator.o Aggregator.o thread.o coro.o stack.o 
Allocator_tests.test: Allocator.o

Collective_tests.test: Delegate.o SoftXMT.o GlobalMemory.o Communicator.o Aggregator.o Timestamp.o thread.o coro.o stack.o Collective.o

thread_tests.test: thread.o stack.o coro.o
thread2_tests.test: SoftXMT.o GlobalMemory.o Communicator.o Aggregator.o Timestamp.o thread.o coro.o stack.o 
thread3_tests.test: SoftXMT.o GlobalMemory.o Communicator.o Aggregator.o Timestamp.o thread.o coro.o stack.o

Cache_tests.test: Cache.o SoftXMT.o Delegate.o GlobalMemory.o GlobalAllocator.o Allocator.o Communicator.o Aggregator.o Timestamp.o thread.o coro.o stack.o 
Rate_measure.exe: SoftXMT.o GlobalMemory.o Delegate.o Communicator.o Aggregator.o Timestamp.o thread.o coro.o stack.o

ForkJoin_tests.test: SoftXMT.o GlobalMemory.o Communicator.o Aggregator.o Delegate.o coro.o stack.o thread.o GlobalAllocator.o Allocator.o

#thread.o: coro.o
#
# Build SoftXMT static library
#
LIBRARY_CONTENTS= SoftXMT.o GlobalAllocator.o GlobalMemory.o Communicator.o Aggregator.o Timestamp.o Allocator.o Cache.o thread.o coro.o stack.o 
libSoftXMT.a: $(patsubst %,libSoftXMT.a(%),$(LIBRARY_CONTENTS))


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
ENV_VARIABLES+= GASNET_BACKTRACE=1
ENV_VARIABLES+= IPATH_PORT=1
ENV_VARIABLES+= GASNET_IBV_PORTS=qib0:1
ENV_VARIABLES+= GASNET_USE_XRC=0



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

#
# run a binary
#
run: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP)
	-killall $<
	$(ENV_VARIABLES) \
	$($(MPITYPE)_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- ./$< $(ARGS)

#
# run tests for a component
#
mpi_test: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP)
	-killall $<
	$(ENV_VARIABLES) \
	$($(MPITYPE)_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- ./$< --log_level=test_suite --run_test=$(TARGET:.test=) -- $(ARGS)

test: $(TARGET)
	$(ENV_VARIABLES) \
	./$< --log_level=test_suite --run_test=$(TARGET:.test=) -- $(ARGS)

# build a specific test
# include Boost test automagic main() generation
# add an additional rule if you need to include multiple components
%_tests.o: CXXFLAGS+= -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=$@
%_tests.test: LIBRARIES+= -lboost_unit_test_framework
%_tests.test: %_tests.o
	$(LD) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBRARIES) -o $@

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
CFLAGS+= --param max-inline-insns-single=35000 --param inline-unit-growth=10000 --param large-function-growth=200000 -Winline
CFLAGS+= $(GASNET_FLAGS)
CFLAGS+= -D_GNU_SOURCE

# not sure if this is a good idea
CFLAGS+= -Wno-inline
#CFLAGS+= -Wall -Wextra

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
# set LD_LIBRARY_PATH
#
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

# build an executable
# add an additional rule if you need to include multiple components
%.exe: %.o 
	$(LD) $(LDFLAGS) $^ $(LIBRARIES) -o $@

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
%.o: %.cpp Makefile
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $< -o $*.o
	$(CXX) -MM $(CFLAGS) $(CXXFLAGS) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

#
# Temporary special handling for thread library
#
thread.o: thread.c Makefile
	$(CC) -c $(CFLAGS) $< -o $*.o
	$(CC) -MM $(CFLAGS) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

coro.o: coro.c Makefile
	$(CC) -c $(CFLAGS) $< -o $*.o
	$(CC) -MM $(CFLAGS) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

stack.o: stack.S Makefile
	$(CC) -c $(CFLAGS) $< -o $*.o


CLEAN_FILES= *.exe *.o *.d *.test *.dis libSoftXMT.a $($(MPITYPE)_CLEAN_FILES)
clean:
	rm -f $(CLEAN_FILES)

.PHONY: run test clean
