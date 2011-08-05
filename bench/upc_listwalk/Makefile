UDP_NUM_THREADS=4
SHM_NUM_THREADS=4

CFLAGS= -g

all: listwalk.default listwalk.udp

listwalk.default: listwalk.c
	upcc $(CFLAGS) -o $@ $< 

listwalk.udp: listwalk.c
	upcc $(CFLAGS) --network udp -T $(UDP_NUM_THREADS) -o $@ $<

run.default: listwalk.default
	GASNET_PSHM_NODES=$(SHM_NUM_THREADS) ./listwalk.default 1024

run.udp: listwalk.udp
	UPC_POLITE_SYNC=0 upcrun -localhost ./listwalk.udp 1024

debug.default: listwalk.default
	GASNET_BACKTRACE=1 GASNET_PSHM_NODES=2 gdb --args ./listwalk.default 1024

clean:
	rm -f listwalk.default listwalk.udp
