EXECS=brandes_serial brandes_1p brandes_1p_coarse brandes_1p_coarse_and_fine gasme \
	atomic_test bwtest task_test brandes_1p_task_clean

all: $(EXECS)

#defualt gasnet library choice
MA_CONDUIT_LIB?=ibv
MA_CONDUIT_DEF?=IBV
MA_THREADING_GASNET_DEF?=PAR
MA_THREADING_GASNET_LIB?=par

GASNET?=/sampa/share/gasnet-rhel6
#MPI_INCLUDE=/usr/include/mpich2
CFLAGS_COMMON=-Wall -I$(GASNET)/include -I$(GASNET)/include/$(MA_CONDUIT_LIB)-conduit -DGASNET_$(MA_THREADING_GASNET_DEF) -DGASNET_CONDUIT_$(MA_CONDUIT_DEF)
CFLAGS=-O3 $(CFLAGS_COMMON)
#CFLAGS=-g -DDEBUG $(CFLAGS_COMMON)
LIBS=
LD=mpicc
CC=mpicc
LFLAGS=$(CFLAGS_COMMON)
GASNET_LIBS=-L$(GASNET)/lib -lgasnet-$(MA_CONDUIT_LIB)-$(MA_THREADING_GASNET_LIB) -libverbs $(MA_ADD_GASNET_LIBS)
#MPI_LIBS=-lmpi
MPI_LIBS=

HEADERS=brandes_common.h fastlist.h base.h msg_aggregator.h global_memory.h global_array.h

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

atomic_test: atomic_test.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

brandes_serial: fastlist.o brandes_serial.o brandes_common.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

brandes_1p: fastlist.o brandes_1p.o brandes_common.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

brandes_1p_coarse: fastlist.o brandes_1p_coarse.o brandes_common.o

brandes_1p_coarse_and_fine: fastlist.o brandes_1p_coarse_and_fine.o brandes_common.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

objects: global_memory.o global_array.o msg_aggregator.o

gasme: gasme.o global_memory.o global_array.o msg_aggregator.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(MPI_LIBS) $(GASNET_LIBS)

bwtest: bwtest.o global_memory.o global_array.o msg_aggregator.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(MPI_LIBS) $(GASNET_LIBS)

task_test.o: tasklib.h task_test.c
	$(CC) -c $(CFLAGS) task_test.c -o $@

task_test: task_test.o tasklib.h
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(EXECS)

archive:
	tar cvf brandes.tar *


