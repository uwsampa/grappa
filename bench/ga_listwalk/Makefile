
CXX=mpic++

CXXFLAGS+= -std=c++0x  -Wno-write-strings -O0 -g

CXXFLAGS+= -I../../tools/ga/include -L../../tools/ga/lib

LIBS+= -lga++ -lga -lgfortran

NUM_JUMP_THREADS?=9
SIZE_LOG?=22
DEBUG?=0

#TARGET=ga_alloc_test.exe
TARGET=jumpwalk_test.exe

OTHERS= ga_alloc.cpp jumpwalk_test.cpp

run: $(TARGET)
	hugectl --heap -- mpiexec -np 6 ./$(TARGET)
#	hugectl --heap -- numactl --membind=0 -- taskset 0x555 mpiexec -np 6 ./$(TARGET)
#	mpiexec -np 1 ./$(TARGET)

ga_alloc_test.exe: ga_alloc.cpp ga_alloc_test.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

jumpwalk_test.cpp: jumpwalk.cunroll

jumpwalk_test.exe: $(OTHERS) jumpwalk.cunroll
	$(CXX) $(CXXFLAGS) -DDEBUG=$(DEBUG) -DSIZE_LOG=$(SIZE_LOG) -DNUM_JUMP_THREADS=$(NUM_JUMP_THREADS) $(OTHERS) -o $@ $(LIBS)

%.cunroll: %.gc ../../jumpthreads/jumpthreads Makefile ../../jumpthreads/preamble.greenery ../../jumpthreads/postamble.greenery ../../jumpthreads/jumpthreads.h 
	../../jumpthreads/jumpthreads $< $@ $(NUM_JUMP_THREADS)

