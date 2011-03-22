
CC=gcc

CFLAGS+=-O3 -g -std=gnu99 -fopenmp -msse3 -fPIC -D_GNU_SOURCE
CPPFLAGS+=-O3 -fPIC  -g -fopenmp -msse3

#CPPFLAGS+=-DCACHELINE_SIZE=64 -DCHUNK_SIZE="(1<<4)" -DQSIZE="(1<<15)"
#CPPFLAGS+=-DCACHELINE_SIZE=64 -DCHUNK_SIZE="(1<<6)" -DQSIZE="(1<<16)"
CPPFLAGS+=-DCACHELINE_SIZE=64
CPPFLAGS+= -I../../liberty-queues

CFLAGS+=-DCACHELINE_SIZE=64
CFLAGS+= -I../../liberty-queues

#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/perf-3.9/lib
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/libpfm-3.9/lib
#CFLAGS+=-I/sampa/home/nelson/libhugetlbfs-2.10 -L/sampa/home/nelson/libhugetlbfs-2.10/obj64
#CFLAGS+=-I../../exper_runner -L../../exper_runner
CFLAGS+= -I../../greenery -L../../greenery

CPPFLAGS+= -I../../greenery -L../../greenery
CPPFLAGS+=-I../../../fastflow_install/include

#CFLAGS+=--hugetlb-text --hugetlb-ptrs
#LIBS+= -lrt -lhugetlbfs
LIBS+=  -lrt -lgreenery

HEADERS=\
	inc.h

OBJECTS=


HOSTNAME=$(shell hostname -A | sed -e 's/ //g')

run_inc: inc2.exe
	 ./$< -d -c 2
#	 ./$< -a -j -o -n -p -d -c 1
#	 ./$< -a -d -j -m -c 1
#	 ./$< -a -j -o -n -p -c 1
#	 ./$< -a -j -o -n -p -c 2
#	GOMP_CPU_AFFINITY="0-11:2,1-11:2" ./$< -a -d
#	CONDOR_REQUIREMENTS='machine == "$(HOSTNAME)"' condor_run ./$<

run_LibertyQueue: LibertyQueue2.exe
	./$<
#	hugectl --heap -- ./$<

run_MCRingBuffer: MCRingBuffer.exe
	./$<

LibertyQueue2.exe: LibertyQueue2.cpp liberty-queues/sw_queue_astream.h Makefile
	$(CXX) $(CPPFLAGS) $< liberty-queues/sw_queue.c -o $@ $(LIBS) 
	objdump -S $@ | tee $@.dis

run_queue: queue.exe
	./$<


inc.exe: inc.cpp Makefile liberty-queues/sw_queue_astream.h
	$(CXX) $(CPPFLAGS) $< liberty-queues/sw_queue.c -o $@ $(LIBS)
	objdump -S $@ | tee $@.dis

inc2.exe: inc2.c Makefile liberty-queues/sw_queue_astream.h
	$(CC) $(CFLAGS) $< liberty-queues/sw_queue.c -o $@ $(LIBS)
	objdump -S $@ | tee $@.dis

run_q: q.exe
	./$<

run_mwait: mwait.exe
	GOMP_CPU_AFFINITY="0,1" ./$<

experiment_inc: inc.exe
	CONDOR_REQUIREMENTS='machine == "$(HOSTNAME)"' condor_run ./inc.sh

objdump_inc: inc.exe
	objdump -S ./$<

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $< -c -o $@ 

%.o: %.cxx $(HEADERS) Makefile
	$(CXX) $(CFLAGS) $< -c -o $@ 

#%.exe: %.c $(HEADERS) $(OBJECTS) Makefile
%.exe: %.c Makefile
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)
	objdump -S $@ | tee $@.dis

%.exe: %.cpp %.hpp Makefile
	$(CXX) $(CPPFLAGS) $< -o $@ $(LIBS)
	objdump -S $@ | tee $@.dis
clean: 
	rm -f inc.exe $(OBJECTS)

.PHONY: run_inc objdump_inc

