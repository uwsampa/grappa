
CC=g++
CFLAGS+=-O2 -fopenmp -g
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/perf-3.9/lib
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/libpfm-3.9/lib
#CFLAGS+=-I/sampa/home/nelson/libhugetlbfs-2.10 -L/sampa/home/nelson/libhugetlbfs-2.10/obj64
CFLAGS+=-I../../exper_runner -L../../exper_runner
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
	walk.o \
	../../ExperimentRunner.o

#run_pfmon_example: self.exe
#	./$<

###
### hacky stuff for jacob's remote development
###
ifeq ($(shell uname -s),Darwin)

CURRENT_BRANCH=$(shell git status | grep branch | sed "s/\# On branch \(.*\)/\1/")
REMOTE_HOST=thing2.cs.washington.edu
REMOTE_DEST=~/softxmt-remote

remote:
	git commit -va
	git push $(REMOTE_HOST):$(REMOTE_DEST) $(CURRENT_BRANCH)
	ssh $(REMOTE_HOST) "cd $(REMOTE_DEST) && git checkout -f $(CURRENT_BRANCH) && cd applications/limits && make"
endif



run_linked_list: linked_list.exe
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 1
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 2
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 3
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 4
	GOMP_CPU_AFFINITY="0-15:2" ./$< 24 8 5

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

