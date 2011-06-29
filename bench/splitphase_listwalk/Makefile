CC=g++
CFLAGS= -g -fopenmp
CFLAGS+= -I../../greenery -L../../greenery -I../../queues -L../../queues -I../../global_memory -L../../global_memory
LIBS= -lrt -lgreenery -lcorequeue -lsplitphase -lbprintf -lnuma

HEADERS=\
	linked_list-node.h \
	linked_list-alloc.h \
	linked_list-walk.h 

OBJECTS=\
	linked_list-node.o \
	linked_list-alloc.o \
	linked_list-walk.o

default: linked_list.exe

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $< -c -o $@

linked_list.exe: linked_list.c $(HEADERS) $(OBJECTS) Makefile
	$(CC) $(CFLAGS) $< -o $@ $(OBJECTS) $(LIBS) ../../queues/MCRingBuffer.c #XXX temp, because undef refs?

all:
	./allmake.sh

run:
	./linked_list.exe -b 14 -t 8 -c 6 -g

clean:
	rm -f linked_list.exe ./*.o
