
CC=gcc
#CFLAGS+=-O2 -fopenmp -g
CFLAGS+=-O2 -g -std=gnu99 -fopenmp
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/perf-3.9/lib
#CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/libpfm-3.9/lib
#CFLAGS+=-I/sampa/home/nelson/libhugetlbfs-2.10 -L/sampa/home/nelson/libhugetlbfs-2.10/obj64
#CFLAGS+=-I../../exper_runner -L../../exper_runner
CFLAGS+= -I../../greenery -L../../greenery
#CFLAGS+=--hugetlb-text --hugetlb-ptrs
#LIBS+= -lrt -lhugetlbfs
LIBS+=  -lrt -lgreenery

HEADERS=\
	linked_list-node.h \
	linked_list-alloc.h \
	linked_list-walk.h

OBJECTS=\
	linked_list-node.o \
	linked_list-alloc.o \
	linked_list-walk.o 
#	../../exper_runner/ExperimentRunner.o



run_linked_list: linked_list.exe
# noop

run_alloc: alloc.exe
	./$<

alloc.exe: linked_list-alloc.c $(HEADERS)
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

