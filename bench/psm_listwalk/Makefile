EXECS=listwalk psm_listwalk

NUM_JUMP_THREADS?=16

run: listwalk
	SSH_SERVERS='n01,n04' ./listwalk 2 -L 24 -n 1000000
#	mpirun -H n01,n04 -np 2 -nonmpi ./$< -c 1 -L 24

#all: $(EXECS)
#run: $(EXECS)
#	mpirun -H n01,n04 -x GASNET_NUM_QPS=1 -np 1 -loadbalance ./$< -L 24 -t $(NUM_JUMP_THREADS)

#GASNET=/home/jnelson/gasnet-mellanox44
#MPI_INCLUDE=/application/openmpi-1.5.3/install-gcc
#GASNET=/sampa/share/gasnet-rhel6
GASNET=/sampa/home/nelson/softxmt/tools/GASNet-1.16.2/gasnet-nompi
MPI_INCLUDE=

#CFLAGS_COMMON= -I$(GASNET)/include -I$(GASNET)/include/mpi-conduit -DGASNET_PAR
CFLAGS_COMMON= -fopenmp -I$(GASNET)/include -I$(GASNET)/include/udp-conduit -DGASNET_PAR -I$(MPI_INCLUDE) -DGASNET_CONDUIT_UDP

CFLAGS_COMMON+= -I../../greenery -L../../greenery
CFLAGS_COMMON+= -I../../queues -L../../queues

CFLAGS=-O2 -g $(CFLAGS_COMMON)
#CFLAGS=-g -DDEBUG $(CFLAGS_COMMON)

LD=c++
CC=cc

LIBS=-lrt -lgomp -lhugetlbfs -lgreenery
LFLAGS=-O2 -g -L../../greenery

#GASNET_LIBS=-L/usr/local/gasnet/lib -lammpi -lgasnet-mpi-par
#GASNET_LIBS=-L$(GASNET)/lib -lgasnet-ibv-par -libverbs
GASNET_LIBS=-L$(GASNET)/lib -lgasnet-udp-par -lamudp
#MPI_LIBS=-lmpi
MPI_LIBS=

CFLAGS+= -I../../jumpthreads -L../../jumpthreads -DNUM_JUMP_THREADS=$(NUM_JUMP_THREADS)

HEADERS=alloc.h node.h options.h

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

%.cunroll: %.gc ../../jumpthreads/jumpthreads Makefile ../../jumpthreads/preamble.greenery ../../jumpthreads/postamble.greenery ../../jumpthreads/jumpthreads.h 
	../../jumpthreads/jumpthreads $< $@ $(NUM_JUMP_THREADS)

PSM_INSTALL=/sampa/home/nelson/softxmt/tools/infinipath-psm-1.14


psm_listwalk.o: jump.cunroll

psm_listwalk: options.o alloc.o psm_listwalk.o walk.o 
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(MPI_LIBS) $(GASNET_LIBS)
	objdump -S $@ > $@.dis

listwalk: options.o alloc.o listwalk.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(MPI_LIBS) $(GASNET_LIBS)
	objdump -S $@ > $@.dis

clean:
	rm -f *.o $(EXECS)

archive:
	tar cvf qnd_listwalk.tar *


