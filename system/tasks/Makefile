# include common variable definitions
include ../../include.mk

SOFTXMT_SYS=..

#
# object file dependencies
#
Thread_tests.test: coro.o stack.o Thread.o BasicScheduler.o


CFLAGS_COMMON= -fopenmp -fomit-frame-pointer
CFLAGS+= $(CFLAGS_COMMON)
CFLAGS+= -I$(GL_ARRAY) -I$(SOFTXMT_SYS)
CFLAGS+= -I..

ifdef DEBUG
CFLAGS+= -O1 -g
else
CFLAGS+= -O3
endif

# supress c char* warnings
CFLAGS+= -Wno-write-strings 

CFLAGS+= -I$(EXPERIMENTS)/lib

CFLAGS+= -DGASNET_SEQ -DGASNET_CONDUIT_IBV
CXXFLAGS=$(CFLAGS)

LIB=-lm 

LDFLAGS+= $(CFLAGS_COMMON) -L$(SOFTXMT_SYS)

LIBRARIES:= -lgflags -lglog -lgasnet-ibv-seq -libverbs -lrt -lboost_unit_test_framework -lSoftXMT

#
# set LD_LIBRARY_PATH
#
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

#Thread_tests.exe: Thread_test.o $(BASIC_OBJECTS)
#	$(CXX) $(LDFLAGS) $^ $(LIBRARIES) -o $@


# build a specific test and update autodependence info
# include Boost test automagic main() generation
%_tests.o: CXXFLAGS+= -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=$@
%_tests.test: LIBRARIES+= -lboost_unit_test_framework
%_tests.test: %_tests.o
	g++ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LIBRARIES) $^ -o $@
	objdump -S $@ > $@.dis

test: $(TARGET)
	$(ENV_VARIABLES) \
	./$< --log_level=test_suite --run_test=$(TARGET:.test=) -- $(ARGS)

clean:
	rm -f ./*.o ./*.test
