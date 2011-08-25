GASNET?=/sampa/share/gasnet-rhel6
GL_ARRAY=../../mark_scratch

CC=mpicxx
LD=mpicxx

#defualt gasnet library choice
MA_CONDUIT_LIB?=ibv
MA_CONDUIT_DEF?=IBV
MA_THREADING_GASNET_DEF?=PAR
MA_THREADING_GASNET_LIB?=par


CFLAGS_COMMON= -Wno-write-strings -O3 -g -fopenmp
CFLAGS= $(CFLAGS_COMMON) 
CFLAGS+= -I../../greenery  -I../../queues  -I../../global_memory  -I../../numautil
CFLAGS+= -I./collective
#CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit -DGASNET_PAR -DGASNET_CONDUIT_IBV 
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(MA_CONDUIT_LIB)-conduit -DGASNET_$(MA_THREADING_GASNET_DEF) -DGASNET_CONDUIT_$(MA_CONDUIT_DEF)
CFLAGS+= -I$(GL_ARRAY) 

LD_FLAGS= $(CFLAGS_COMMON) 
LIB_PATHS= -L../../greenery -L../../queues -L../../global_memory -L$(GL_ARRAY)
LIBS= -lrt -lgreenery -lcorequeue -lsplitphase 

PWD=$(shell pwd)

GASNET_LIB_PATHS= -L$(GASNET)/lib
#GASNET_LIBS= -lgasnet-ibv-par -libverbs
GASNET_LIBS= -lgasnet-$(MA_CONDUIT_LIB)-$(MA_THREADING_GASNET_LIB) -libverbs $(MA_ADD_GASNET_LIBS)

HEADERS=\
	node.hpp \
	linked_list-walk.h \
	collective/collective.h

OBJECTS=\
	linked_list-walk.o \
	linked_list-alloc.o \
	collective/collective.o

#../../numautil/numautil.o \

GL_OBJECTS=\
	$(GL_ARRAY)/global_memory.o \
	$(GL_ARRAY)/global_array.o \
	$(GL_ARRAY)/msg_aggregator.o

default: linked_list.exe

run: linked_list.exe
	mpirun -np 2 -H "n01,n04" hugectl --heap -- ./linked_list.exe -b 10 -c 1 -t 4 -l 1 -g -n 2

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $< -c -o $@

linked_list.exe: linked_list.o $(OBJECTS) $(GL_OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LIB_PATHS) $(GASNET_LIB_PATHS) $(LIBS) $(GASNET_LIBS) 

all:
	./allmake_gas.sh


debug: linked_list.exe 
	export GASNET_BACKTRACE=1
	mpirun -np 2 -H "n01" xterm -hold -e gdb --args ./linked_list.exe -b 10 -l 1 -c 1 -g -t 2 -n 1

clean:
	rm -f linked_list.exe ./*.o
