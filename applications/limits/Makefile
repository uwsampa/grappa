
CC=g++
CFLAGS+=-O -fopenmp -g
CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/perf-3.9/lib
CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/libpfm-3.9/lib
CFLAGS+=-I/sampa/home/nelson/libhugetlbfs-2.10 -L/sampa/home/nelson/libhugetlbfs-2.10/obj64
#CFLAGS+=--hugetlb-text --hugetlb-ptrs
#LIBS+= -lrt -lhugetlbfs
LIBS+= -lrt

HEADERS=\
	node.h \
	alloc.h \
	walk.h

OBJECTS=\
	node.o \
	alloc.o \
	walk.o

#run_pfmon_example: self.exe
#	./$<

###
### hacky stuff for jacob's remote development
###
ifeq ($(shell uname -s),Darwin)

CURRENT_BRANCH=integrate-limit2
REMOTE_HOST=thing2
REMOTE_DEST=~/softxmt-remote

remote:
	git commit -va
	git push $(REMOTE_HOST):$(REMOTE_DEST) $(CURRENT_BRANCH)
	ssh $(REMOTE_HOST) "cd $(REMOTE_DEST) && git checkout -f $(CURRENT_BRANCH) && cd applications/limits && make"
endif



run_linked_list: linked_list.exe
	#LD_LIBRARY_PATH=/sampa/home/nelson/libhugetlbfs-2.10/obj64 HUGETLB_MORECORE=yes taskset 0x00000001 ./$< 22 1 1
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 1
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 2
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 3
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 4
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 5
	#taskset 0x00005555 ./$< 24 1 1
	#taskset 0x00005555 ./$< 22 5 4
	#taskset 0x00000005 ./$< 22 10 2
	#taskset 0x00000001 ./$< 22 2 1
	#taskset 0x00000001 ./$< 22 3 1
	#taskset 0x00000001 ./$< 22 4 1
	#taskset 0x00000001 ./$< 22 5 1
#	taskset 0x00000001 ./$< 22 12 1
	#taskset 0x00005555 ./$< 22 5 8

run_alloc: alloc.exe
	./$<

alloc.exe: alloc.c $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@

objdump_linked_list: linked_list.exe
	objdump -S ./$<

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $< -c -o $@ 

%.exe: %.c $(HEADERS) $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $< -o $@ $(OBJECTS) $(LIBS)

clean: 
	rm -f linked_list.exe

.PHONY: run_linked_list objdump_linked_list run_pfmon run_alloc

