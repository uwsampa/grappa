EXECS=listwalk

NUM_JUMP_THREADS?=16

run: listwalk
#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 20:2 -L 18 -n 3 -t 128
#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 20:2 -L 21 -n 16 -t 256 -c 10 -b 35

#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 20:2 -L 21 -n 1 -t 256 -b 20 -c 10
#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 2:2 -L 21 -n 1 -t 256 -b 20 -c 1 --type inline --payload_size_log 3

	GASNET_SSH_SERVERS='n01,n04' ./listwalk 2:2 -L 15 -n 1 -t 256 -b 50 -c 1 --type block --payload_size_log 7
#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 20:2 -L 21 -n 1 -t 256 -b 50 -c 10 --type block --payload_size_log 7
#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 20:2 -L 21 -n 1 -t 256 -b 10 -c 10 --type inline --payload_size_log 5

#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 2:2 -L 18 -n 1 -t 1 -b 1 -c 1 --type block --payload_size_log 0

#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 2:2 -L 21 -n 1 -t 8
#	GASNET_SSH_SERVERS='n01,n04' ./listwalk 2:2 -L 12 -n 4 -t 1
##	GASNET_SSH_SERVERS='n01,n04' ./listwalk 2:2 -L 21 -n 1 -t 1
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
CFLAGS_COMMON= -fopenmp -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit -DGASNET_PAR -I$(MPI_INCLUDE) -DGASNET_CONDUIT_IBV
#CFLAGS_COMMON= -fopenmp -I$(GASNET)/include -I$(GASNET)/include/udp-conduit -DGASNET_PAR -I$(MPI_INCLUDE) -DGASNET_CONDUIT_UDP

CFLAGS_COMMON+= -I../../greenery -L../../greenery

CFLAGS_COMMON+= -I../../queues -L../../queues
CFLAGS_COMMON+= -DMCRINGBUFFER_CACHE_LINE_SIZE=64 -DMCRINGBUFFER_BATCH_SIZE=64 -DMCRINGBUFFER_SIZE_LOG=20
PSM_INSTALL=/sampa/home/nelson/softxmt/tools/infinipath-psm-1.14
CFLAGS_COMMON+= -I$(PSM_INSTALL) -L$(PSM_INSTALL)/usr/lib64

CFLAGS_COMMON+= -I/sampa/home/nelson/softxmt/tools/GASNet-1.16.2/other/ssh-spawner

CFLAGS=-O2 -g $(CFLAGS_COMMON)
#CFLAGS=-g -DDEBUG $(CFLAGS_COMMON)

LD=cc
CC=cc

LIBS=-lrt -lgomp -lhugetlbfs -lgreenery -lpsm_infinipath -luuid
LFLAGS=-O2 -g -L../../greenery

#GASNET_LIBS=-L/usr/local/gasnet/lib -lammpi -lgasnet-mpi-par
GASNET_LIBS=-L$(GASNET)/lib -lgasnet-ibv-par -libverbs
#GASNET_LIBS=-L$(GASNET)/lib -lgasnet-udp-par -lamudp
#MPI_LIBS=-lmpi
MPI_LIBS=

CFLAGS+= -I../../jumpthreads -L../../jumpthreads -DNUM_JUMP_THREADS=$(NUM_JUMP_THREADS)

HEADERS=alloc.h node.h options.h stack.h MCRingBuffer.h option_macros.h metrics.h

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

%.cunroll: %.gc ../../jumpthreads/jumpthreads Makefile ../../jumpthreads/preamble.greenery ../../jumpthreads/postamble.greenery ../../jumpthreads/jumpthreads.h 
	../../jumpthreads/jumpthreads $< $@ $(NUM_JUMP_THREADS)

PSM_INSTALL=/sampa/home/nelson/softxmt/tools/infinipath-psm-1.14




listwalk.o: CFLAGS+= -I/sampa/home/nelson/softxmt/tools/GASNet-1.16.2/other/ssh-spawner
#listwalk: options.o alloc.o listwalk.o psm_utils.o MCRingBuffer.o option_macros.o
listwalk: alloc.o listwalk.o psm_utils.o MCRingBuffer.o option_macros.o
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) $(MPI_LIBS) $(GASNET_LIBS)
	objdump -S $@ > $@.dis







clean:
	rm -f *.o $(EXECS)

archive:
	tar cvf qnd_listwalk.tar *


