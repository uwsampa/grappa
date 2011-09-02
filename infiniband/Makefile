.PHONY: clean

LD	:= cc
#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g -O3 -fopenmp -Wno-unused-variable
LDFLAGS := ${LDFLAGS} -lrt -lhugetlbfs -lgomp -libverbs

#APPS    := small_messages mpi_throughput #info4
APPS    := ibv_contexts

#SALLOC_ARGS := --cpu_bind=verbose,none --mem_bind=verbose,none -n 2 -N 1-1 --ntasks-per-socket=1
SALLOC_ARGS := -n 2 -N 2-2 --ntasks-per-socket=1 --exclusive
#MPI_RUNTIME_ARGS :=  --mca btl ^openib --mca btl_tcp_if_include eth0 
MPI_RUNTIME_ARGS :=

all: ${APPS}
	-ssh n02 killall -w $^
	-ssh n04 killall -w $^
	mpirun -k 5 -q 5 -H n02,n04 -np 2 $(PWD)/$^ --messages --count=1000000 --batch_size=8 --outstanding=1024 --payload_size_log=0

#	mpirun -mca btl ^openib -np 2 $(PWD)/$^ --rdma_write --payload_size_log 10  --count 10000 --cores 1
#	mpirun -mca btl ^openib -np 2 $(PWD)/$^ --messages --payload_size_log 3  --count 10000000 --outstanding 10000  --cores 1
#	mpirun --host n04 --host n02 -np 2 --loadbalance $(PWD)/$^ --messages --payload_size_log 4  --count 100000 --outstanding 10000  --cores 1
#	salloc $(SALLOC_ARGS) mpirun $(MPI_RUNTIME_ARGS) $(PWD)/$^

info4: info4.o find_ib_interfaces.o
	${LD} -o $@ $^ ${LDFLAGS}

small_messages: CC=mpicc
small_messages: LD=mpicc
#small_messages: small_messages.o find_ib_interfaces.o rdma.o alloc.o
small_messages: small_messages.o alloc.o mpi_utils.o ib_utils.o options.o
	${LD} -o $@ $^ ${LDFLAGS}

ibv_contexts: CC=mpicc
ibv_contexts: LD=mpicc
ibv_contexts: ibv_contexts.o alloc.o mpi_utils.o options.o send_batch.o
	${LD} -o $@ $^ ${LDFLAGS}

mpi_throughput: CC=mpicc
mpi_throughput: LD=mpicc 
mpi_throughput: mpi_throughput.o mpi_utils.o options.o alloc.o
	${LD} -o $@ $^ ${LDFLAGS}

PSM_INSTALL=/sampa/home/nelson/softxmt/tools/infinipath-psm-1.14
#psmtest: CC=mpicc
#psmtest: LD=mpicc
psmtest: LDFLAGS+= -L$(PSM_INSTALL)/usr/lib64 -lpsm_infinipath -luuid
psmtest: CFLAGS+= -I$(PSM_INSTALL)/usr/include -lpsm_infinipath -luuid
psmtest: psmtest.o options.o
	${LD} -o $@ $^ ${LDFLAGS}


clean:
	rm -f *.o ${APPS}

