CC=gcc
CFLAGS+=-fopenmp -g -std=gnu99 -O3 -fPIC -D_GNU_SOURCE
LIBS+=-lrt -lhugetlbfs

CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64
#CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64 -DCHUNK_SIZE="(1<<4)" -DQSIZE="(1<<20)"

CFLAGS+= -I../../greenery -L../../greenery
LIBS+= -lgreenery

gups: gups.c Makefile
	$(CC) $(CFLAGS) $< ../../liberty-queues/sw_queue.c -o $@ $(LIBS)

clean:
	rm -f gups
