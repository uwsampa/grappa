CC=mpic++
CFLAGS= -Wno-write-strings -O3 -g -fopenmp
CFLAGS+= -I../../greenery -L../../greenery -I../../queues -L../../queues -I../../global_memory -L../../global_memory -I../../numautil
LIBS= -lrt -lgreenery -lcorequeue -lsplitphase -lbprintf -lnuma 

HOSTNAME=$(shell hostname)
PWD=$(shell pwd)

# for GA
CFLAGS+= -I../../tools/ga-5-0-2/ga-$(HOSTNAME)/include -L../../tools/ga-5-0-2/ga-$(HOSTNAME)/lib -I../ga_listwalk
LIBS+= -lga++ -lga -lgfortran #-libverbs # -llapack


HEADERS=\
	node.hpp \
	linked_list-walk.h 

OBJECTS=\
	linked_list-walk.o \
	../../numautil/numautil.o \
	../ga_listwalk/ga_alloc.o

default: linked_list.exe

run: run_$(HOSTNAME)

run_n01: linked_list.exe
	mpiexec -np 6 hugectl --heap  -- ./linked_list.exe -b 10 -c 1 -t 4 -l 1 -g

run_n02: linked_list.exe
	mpiexec -np 6 hugectl ---heap -- ./linked_list.exe -b 10 -c 1 -t 4 -l 1 -g

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $< -c -o $@

linked_list.exe: linked_list.c $(HEADERS) $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $< -o $@ $(OBJECTS) $(LIBS) ../../queues/MCRingBuffer.c #XXX temp, because undef refs?

all:
	./allmake.sh


debug: debug_$(HOSTNAME)

#need x session
debug_n01: linked_list.exe 
	mpiexec -np 2 xterm -hold -e gdb --args ./linked_list.exe -b 12 -l 1 -c 2 -g -t 2    

clean:
	rm -f linked_list.exe ./*.o
