
CXX=mpic++

CXXFLAGS+= -Wno-write-strings -O2 -g

CXXFLAGS+= -I../../tools/ga-ib/include -L../../tools/ga-ib/lib

LIBS+= -lga++ -lga -lgfortran -libverbs -lrt -llapack

NUM_JUMP_THREADS?=64
SIZE_LOG?=19
DEBUG?=0

HOSTNAME=$(shell hostname)
PWD=$(shell pwd)

#TARGET=ga_alloc_test.exe
TARGET=jumpwalk_test.exe

OTHERS= ga_alloc.cpp jumpwalk_test.cpp

run: run_$(HOSTNAME)

run_catamount: $(TARGET)
	salloc  --cpu_bind=verbose,none --mem_bind=verbose,none -n 8 -N 2-2 --ntasks-per-socket=2 mpirun --mca btl ^openib --mca btl_tcp_if_include eth0 $(PWD)/jumpwalk_test.exe

run_n01: $(TARGET)
	mpiexec -np 6 hugectl --heap -- numactl --membind=0 -- taskset 0x555 ./$(TARGET)

#	hugectl --heap -- mpiexec -np 6 ./$(TARGET)
#	hugectl --heap -- numactl --membind=0 -- taskset 0x555 mpiexec -np 6 ./$(TARGET)
#	mpiexec -np 1 ./$(TARGET)

ga_alloc_test.exe: ga_alloc.cpp ga_alloc_test.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

jumpwalk_test.cpp: jumpwalk.cunroll

jumpwalk_test.exe: $(OTHERS) jumpwalk.cunroll
	$(CXX) $(CXXFLAGS) -DDEBUG=$(DEBUG) -DSIZE_LOG=$(SIZE_LOG) -DNUM_JUMP_THREADS=$(NUM_JUMP_THREADS) $(OTHERS) -o $@ $(LIBS)

%.cunroll: %.gc ../../jumpthreads/jumpthreads Makefile ../../jumpthreads/preamble.greenery ../../jumpthreads/postamble.greenery ../../jumpthreads/jumpthreads.h 
	../../jumpthreads/jumpthreads $< $@ $(NUM_JUMP_THREADS)

