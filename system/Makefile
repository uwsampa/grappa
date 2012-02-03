#
# Makefile for SoftXMT V0
#
# This provides automatic header dependence discovery, 
# as well as an easiery way to run tests
#
# Simple principles:
#  - Executables are named <name>.exe.
#  - Test executables are named <name>.test. 
#  - Tests are run using MPI.
#  - Each .cpp file without a main() can include tests and be used to
#    make a .test
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

# name of main executable to build and/or run
TARGET:= Addressing.test

# default rule: just build
$(TARGET): 

#
# object file dependences
#
Delegate.test: Communicator.o Aggregator.o
Addressing.test: Communicator.o Aggregator.o
Aggregator.test: Communicator.o
Communicator.test:

#
# run a binary
#
run: $(TARGET)
	OMPI_MCA_btl_sm_use_knem=0 \
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" \
	mpirun -x LD_LIBRARY_PATH -H $(HOST) -np 2 -- ./$< $(ARGS)

#
# run tests for a component
#
test: $(TARGET)
	OMPI_MCA_btl_sm_use_knem=0 \
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" \
	GASNET_BACKTRACE=1 \
	mpirun -x LD_LIBRARY_PATH -H $(HOST) -np 2 -- ./$< --log_level=test_suite --run_test=$(TARGET:%.test=%_tests)

# build a specific test and update autodependence info
# include Boost test automagic main() generation
%.test: LDFLAGS+= -lboost_unit_test_framework -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=$@
%.test: %.cpp
	$(LD) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBRARIES) -o $@
	$(CXX) -MM $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

#
# What libraries do we need?
#
LIBRARIES:= -lgflags -lgasnet-ibv-seq -libverbs -lrt -lboost_unit_test_framework -lgreenery

#
# set compiler flags
#
CFLAGS+= --param max-inline-insns-single=35000 --param inline-unit-growth=10000 --param large-function-growth=200000 -Winline
CFLAGS+= -DGASNET_SEQ -DGASNET_CONDUIT_IBV
CFLAGS+= -D_GNU_SOURCE
CFLAGS+= -DBOOST_TEST_DYN_LINK

# not sure if this is a good idea
CFLAGS+= -Wno-inline

#
# set debug flags
#
ifndef DEBUG
CFLAGS+= -O1 -g -DDEBUG
else
CFLAGS+= -O3
endif


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
%.o: %.cpp
	$(CXX) -c $(CFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

clean:
	rm -f *.exe *.o *.d *.test
#	rm -f $(TARGET) $(OBJECTS) $(OBJECTS:.o=.d) $(OBJECTS:.o=.test) $(TEST_TARGET)
