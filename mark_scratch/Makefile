EXECS=brandes_serial brandes_1p brandes_1p_coarse brandes_1p_coarse_and_fine gasme \
	atomic_test

all: $(EXECS)

CFLAGS_COMMON=-Wall -I/usr/local/gasnet/include -I/usr/local/gasnet/include/mpi-conduit -DGASNET_PAR
#CFLAGS=-O3 $(CFLAGS_COMMON)
CFLAGS=-g -DDEBUG $(CFLAGS_COMMON)
LIBS=
LD=gcc
CC=gcc
LFLAGS=
GASNET_LIBS=-L/usr/local/gasnet/lib -lammpi -lgasnet-mpi-par
MPI_LIBS=-lmpi

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

gasme: gasme.o global_memory.o global_array.o msg_aggregator.o
	$(LD) $(LFLAGS) $(MPI_LIBS) $(GASNET_LIBS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(EXECS)

archive:
	tar cvf brandes.tar *
