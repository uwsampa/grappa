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
include ../include.mk

#
# Define DEBUG to build debug versions of binaries/tests
# This may be done at the command line: "make TARGET=foo.test DEBUG=1 test"
#
#DEBUG:=1

#
# name of host on which to run tests
#
HOST:=localhost

#
# number of mpi processes
#
NPROC:=2

# name of main executable to build and/or run
TARGET:= Addressing.test

# default rule: just build
$(TARGET): 

#
# object file dependences
#
Demo.exe: SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o

Communicator_tests.test:

Aggregator_tests.test: Communicator.o Aggregator.o
MutableHeap.test:


Delegate_tests.test: Delegate.o SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o 
Addressing_tests.test: SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o 
GlobalMemory_tests.test: Delegate.o SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o 
Allocator_tests.test: Allocator.o

Collective_tests.test: Delegate.o SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o Collective.o

thread_tests.test: thread.o stack.o coro.o
thread2_tests.test: SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o 
thread3_tests.test: SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o

Cache_tests.test: Cache.o SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o 

Rate_measure.exe: SoftXMT.o Delegate.o Communicator.o Aggregator.o thread.o coro.o stack.o

#thread.o: coro.o
#
# Build SoftXMT static library
#
LIBRARY_CONTENTS= SoftXMT.o Communicator.o Aggregator.o thread.o coro.o stack.o 
libSoftXMT.a: $(patsubst %,libSoftXMT.a(%),$(LIBRARY_CONTENTS))


#
# environment variables for run
# remember to include environment's $LD_LIBRARY_PATH, since MPI libs may be there
#
ENV_VARIABLES+= GLOG_logtostderr=1
ENV_VARIABLES+= GLOG_v=1
ENV_VARIABLES+= OMPI_MCA_btl_sm_use_knem=0
ENV_VARIABLES+= LD_LIBRARY_PATH="\$$LD_LIBRARY_PATH:$(LD_LIBRARY_PATH)"
ENV_VARIABLES+= GASNET_BACKTRACE=1

#
# how do we export environment variables
#
OPENMPI_EXPORT_ENV_VARIABLES=$(patsubst %,-x %,$(patsubst DELETEME:%,,$(subst =, DELETEME:,$(ENV_VARIABLES))))

#
# temporary file for environment variables for QLogic MPI
# 

# create a temporary filename for environment variables
ENVVAR_TEMP:=$(shell mktemp --dry-run --tmpdir=. .mpirunrc.XXXXXXXX )
# command fragment to use environment variables
QLOGIC_EXPORT_ENV_VARIABLES=-rcfile $(ENVVAR_TEMP)

# create an environment variable file when needed
.mpirunrc.%:
	echo \#!/bin/bash > $@
	echo "if [ -e ~/.mpirunrc ]; then . ~/.mpirunrc; fi" >> $@
	for i in $(ENV_VARIABLES); do echo export $$i >> $@; done

# delete when done
.INTERMEDIATE: $(ENVVAR_TEMP)

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
run: $(TARGET) $(ENVVAR_TEMP)
	-killall $<
	$(ENV_VARIABLES) \
	$(MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) -H $(HOST) -np 2 -- ./$< $(ARGS)

#
# run tests for a component
#
mpi_test: $(TARGET) $(ENVVAR_TEMP)
	-killall $<
	$(ENV_VARIABLES) \
	$(MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) -H $(HOST) -np $(NPROC) -- ./$< --log_level=test_suite --run_test=$(TARGET:.test=) -- $(ARGS)

test: $(TARGET)
	$(ENV_VARIABLES) \
	./$< --log_level=test_suite --run_test=$(TARGET:.test=) -- $(ARGS)

# build a specific test and update autodependence info
# include Boost test automagic main() generation
%_tests.o: CXXFLAGS+= -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=$@
%_tests.test: LIBRARIES+= -lboost_unit_test_framework
%_tests.test: %_tests.o
	$(LD) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LIBRARIES) $^ -o $@
	objdump -S $@ > $@.dis

#
# What libraries do we need?
#
LIBRARIES+= -lgflags -lglog -lgasnet-ibv-seq -libverbs -lrt

#
# set compiler flags
#
CFLAGS+= --param max-inline-insns-single=35000 --param inline-unit-growth=10000 --param large-function-growth=200000 -Winline
CFLAGS+= -DGASNET_SEQ -DGASNET_CONDUIT_IBV
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
	objdump -S $@ > $@.dis

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


clean:
	rm -f *.exe *.o *.d *.test *.btr .mpirunrc.* *.dis
#	rm -f $(TARGET) $(OBJECTS) $(OBJECTS:.o=.d) $(OBJECTS:.o=.test) $(TEST_TARGET)


.PHONY: run test clean
