CC=gcc
<<<<<<< HEAD
CFLAGS+=-fopenmp -g -std=gnu99 -O1
=======
CFLAGS+=-fopenmp -g -std=gnu99 -O3 -fPIC -D_GNU_SOURCE
LIBS+=-lrt -lhugetlbfs
>>>>>>> 8e4922b9c348a08297364823d625e5f3d38180ba

CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64
#CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64 -DCHUNK_SIZE="(1<<4)" -DQSIZE="(1<<20)"

gups: gups.c Makefile
	$(CC) $(CFLAGS) $< ../../liberty-queues/sw_queue.c -o $@ $(LIBS)

clean:
	rm -f gups
