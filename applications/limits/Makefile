
CFLAGS+=-O -fopenmp -g
CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/perf-3.9/lib
CFLAGS+=-I/nvtmp/perfmon2/libpfm-3.9/include -L/nvtmp/perfmon2/libpfm-3.9/lib
CFLAGS+=-I/sampa/home/nelson/libhugetlbfs-2.10 -L/sampa/home/nelson/libhugetlbfs-2.10/obj64
#CFLAGS+=--hugetlb-text --hugetlb-ptrs
#LIBS+= -lrt -lhugetlbfs
LIBS+= -lrt

HEADERS=\
	node.h \
	alloc.h 

OBJECTS=\
	node.o \
	alloc.o

#run_pfmon_example: self.exe
#	./$<

run_linked_list: linked_list.exe
	#LD_LIBRARY_PATH=/sampa/home/nelson/libhugetlbfs-2.10/obj64 HUGETLB_MORECORE=yes taskset 0x00000001 ./$< 22 1 1
	taskset 0x00005555 ./$< 24 6 8
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
	gcc $(CFLAGS) $< -o $@

objdump_linked_list: linked_list.exe
	objdump -S ./$<

%.o: %.c $(HEADERS) Makefile
	gcc $(CFLAGS) $< -c -o $@ 

%.exe: %.c $(HEADERS) $(OBJECTS) Makefile
	gcc $(CFLAGS) $< -o $@ $(OBJECTS) $(LIBS)

clean: 
	rm -f linked_list.exe

.PHONY: run_linked_list objdump_linked_list run_pfmon run_alloc

