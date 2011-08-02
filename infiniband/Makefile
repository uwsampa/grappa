.PHONY: clean

LD	:= cc
#CFLAGS  := -Wall -Werror -g
CFLAGS  := -Wall -g
LDFLAGS := ${LDFLAGS} -lrdmacm -lrt

APPS    := small_messages info4

#SALLOC_ARGS := --cpu_bind=verbose,none --mem_bind=verbose,none -n 2 -N 1-1 --ntasks-per-socket=1
SALLOC_ARGS := -n 2 -N 2-2 --ntasks-per-socket=1 --exclusive
#MPI_RUNTIME_ARGS :=  --mca btl ^openib --mca btl_tcp_if_include eth0 
MPI_RUNTIME_ARGS :=

all: ${APPS}
	salloc $(SALLOC_ARGS) mpirun $(MPI_RUNTIME_ARGS) $(PWD)/$^

info4: info4.o find_ib_interfaces.o
	${LD} -o $@ $^ ${LDFLAGS}

small_messages: CC=mpicc
small_messages: LD=mpicc
#small_messages: small_messages.o find_ib_interfaces.o rdma.o alloc.o
small_messages: small_messages.o find_ib_interfaces.o alloc.o use_verbs.o
	${LD} -o $@ $^ ${LDFLAGS}

clean:
	rm -f *.o ${APPS}

