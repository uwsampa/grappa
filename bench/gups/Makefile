CC=gcc
CFLAGS+=-fopenmp -g -std=gnu99 -O3 -fPIC -D_GNU_SOURCE
LIBS+=-lrt -lhugetlbfs

#CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64
CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64 -DCHUNK_SIZE="(1<<4)" -DQSIZE="(1<<18)"

CFLAGS+= -I../../greenery -L../../greenery -I../../exper_runner
LIBS+= -lgreenery -llikwid

OBJECTS=\
    ../../exper_runner/ExpRun.o

all: gups er-gups

gups: gups.c Makefile ../../liberty-queues/sw_queue_greenery.h
	$(CC) $(CFLAGS) $< ../../liberty-queues/sw_queue.c -o $@ $(LIBS)

er-gups: er-gups.c Makefile ../../liberty-queues/sw_queue_greenery.h $(OBJECTS)
	$(CC) $(CFLAGS) $< $(OBJECTS) ../../liberty-queues/sw_queue.c -o $@ $(LIBS)


clean:
	rm -f gups gupsn er-gups
