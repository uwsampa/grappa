
CC=mpicc
#CC=c++


#CFLAGS+=-O2 -fopenmp -g
CFLAGS+=-O1 -g -std=gnu99 -fopenmp
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/perf-3.9/lib
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/libpfm-3.9/lib
#CFLAGS+=-I/sampa/home/nelson/libhugetlbfs-2.10 -L/sampa/home/nelson/libhugetlbfs-2.10/obj64
#CFLAGS+=-I../../exper_runner -L../../exper_runner
CFLAGS+= -I../../greenery -L../../greenery
CFLAGS+= -I../../jumpthreads -L../../jumpthreads
CFLAGS+= -I../../queues -L../../queues
#CFLAGS+=--hugetlb-text --hugetlb-ptrs
#LIBS+= -lrt -lhugetlbfs
LIBS+=  -lrt -lgreenery

NUM_CORES ?= 4
NUM_THREADS ?= 8
LOG_SIZE ?= 24

#CFLAGS+= -DDEBUG=1

CFLAGS+=-DNUM_JUMP_THREADS=$(NUM_THREADS)
HEADERS=\
	../../jumpthreads/jumpthreads.h \
	../../queues/MCRingBuffer.h \
	linked_list-delegate.h \
	linked_list-node.h \
	linked_list-alloc.h \
	linked_list-walk.h \
	linked_list-jumpthreads.cunroll \
	delegate-jump.cunroll

OBJECTS=\
	../../queues/MCRingBuffer.o \
	linked_list-delegate.o \
	linked_list-node.o \
	linked_list-alloc.o \
	linked_list-walk.o 

#	../../exper_runner/ExperimentRunner.o


default: linked_list.exe linked_list.dis

jacob_target: linked_list.exe linked_list.dis
	GOMP_CPU_AFFINITY="0-12:2" numactl --membind=0 -- hugectl --heap -- ./linked_list.exe -b $(LOG_SIZE) -l $(NUM_THREADS) -c $(NUM_CORES) -j


run_linked_list: linked_list-mpi.exe
	mpdboot --totalnum=-1 --ncpus=48
	mpiexec -n 4 ./linked_list-mpi.exe
	mpdallexit

run_alloc: alloc.exe
	./$<

alloc.exe: linked_list-alloc.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

%.dis: %.exe
	objdump -S ./$< > $@

linked_list-walk.o: linked_list-jumpthreads.cunroll delegate-jump.cunroll

%.o: %.c $(HEADERS) Makefile 
	$(CC) $(CFLAGS) $< -c -o $@ 

%.exe: %.c $(HEADERS) $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $< -o $@ $(OBJECTS) $(LIBS)


linked_list-jumpthreads.cunroll: NUM_JUMP_THREADS=$(NUM_THREADS)

delegate-jump.cunroll: NUM_JUMP_THREADS=$(NUM_CORES)

%.cunroll: %.gc ../../jumpthreads/jumpthreads Makefile ../../jumpthreads/preamble.greenery ../../jumpthreads/postamble.greenery ../../jumpthreads/jumpthreads.h 
	../../jumpthreads/jumpthreads $< $@ $(NUM_JUMP_THREADS)

clean: 
	rm -f linked_list.exe

.PHONY: run_linked_list objdump_linked_list run_pfmon run_alloc

