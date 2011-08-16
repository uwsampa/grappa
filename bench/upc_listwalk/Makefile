UDP_NUM_THREADS=4
SHM_NUM_THREADS=4
IBV_NUM_THREADS=2
IBV_TH_PER_NODE=$(shell echo $(IBV_NUM_THREADS)/2 | bc)

HOSTNAME=$(shell hostname)

CFLAGS= -g

all: listwalk.default listwalk.udp

listwalk.default: listwalk.c
	upcc $(CFLAGS) -o $@ $< 

listwalk.ibv: listwalk.c
	upcc $(CFLAGS) --network ibv -T $(IBV_NUM_THREADS) -o $@ $<

opt.listwalk.ibv: listwalk.c
	upcc $(CFLAGS) -opt --network ibv -T $(IBV_NUM_THREADS) -o $@ $<

trans: listwalk.c
	upcc $(CFLAGS) -opt -trans --network ibv -T $(IBV_NUM_THREADS) -o opt.listwalk.trans.c $<
	upcc $(CFLAGS) -trans --network ibv -T $(IBV_NUM_THREADS) -o listwalk.trans.c $<

view-trans: trans
	vim -O opt.listwalk.trans.c listwalk.trans.c listwalk.c	

listwalk.udp: listwalk.c
	upcc $(CFLAGS) --network udp -T $(UDP_NUM_THREADS) -o $@ $<

run: run.$(HOSTNAME)

run.default: listwalk.default
	GASNET_PSHM_NODES=$(SHM_NUM_THREADS) ./listwalk.default 

run.n01: listwalk.default
	./listwalk.default 

run.udp: listwalk.udp
	UPC_POLITE_SYNC=0 upcrun -localhost ./listwalk.udp 

run.ibv: listwalk.ibv
	export UPC_NODES="n02.sampa n04.sampa"
	upcrun -N 2 -c $(IBV_TH_PER_NODE) ./listwalk.ibv

run.opt.ibv: opt.listwalk.ibv
	export UPC_NODES="n02.sampa n04.sampa"
	upcrun -N 2 -c $(IBV_TH_PER_NODE) ./opt.listwalk.ibv

debug.default: listwalk.default
	GASNET_BACKTRACE=1 GASNET_PSHM_NODES=2 gdb --args ./listwalk.default 

clean:
	rm -f listwalk.default listwalk.udp listwalk.ibv opt.listwalk.ibv *.trans.c listwalk.o
