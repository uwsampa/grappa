CC=gcc
CFLAGS+= -std=gnu99 -g -fopenmp

# DQSIZE default 1<<18
CFLAGS+= -I../../liberty-queues -DCACHELINE_SIZE=64 

CMDL_FLAGS= -DCHUNK_SIZE="(1<<4)" -DQSIZE="(1<<15)" -DSTREAM="1"

all: twowayq twowaydel onewayq

twowayq: twowayq.c ../../liberty-queues/sw_queue.c Makefile
	$(CC) $(CMDL_FLAGS) $(CFLAGS) ../../liberty-queues/sw_queue.c twowayq.c -o twowayq

twowaydel: twowaydel.c ../../liberty-queues/sw_queue.c Makefile
	$(CC) $(CMDL_FLAGS) $(CFLAGS) ../../liberty-queues/sw_queue.c twowaydel.c -o twowaydel

onewayq: onewayq.c ../../liberty-queues/sw_queue.c Makefile
	$(CC) $(CMDL_FLAGS) $(CFLAGS) ../../liberty-queues/sw_queue.c onewayq.c -o onewayq

clean:
	rm -f twowayq twowaydel onewayq
