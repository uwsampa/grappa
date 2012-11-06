# makefile

# SGI UV shmem
CC = mpicc 
#CFLAGS = -I. 
CFLAGS = -I. -I/opt/sgi/mpt/mpt-2.01/include
LFLAGS = -lsma 
SUFFIX = shmem.exe

# SGI UV upc
#CC = sgiupc -w
#CFLAGS = -I.
#LFLAGS = 
#SUFFIX = upc.exe

# CRAY XT5 upc 
#  Need to use Cray compiler (or Berkeley)
#CC = cc
#CFLAGS = -h upc -D__UPC__ -I. -I.. 
#LFLAGS =
#SUFFIX = upc.exe 

# HP XC SHMEM
#LFLAGS = -lshmem -lm
#CC = icc
#CFLAGS = -std=c99 -O3 -w1 -g -I.
#CC = gcc
#CFLAGS = -std=gnu99 -O3 -g -I.

# HP XC UPC
#LFLAGS = -lm
#CC = upc
#CFLAGS = -O3 -upc_backend icc -I.

# HP SMP SHMEM
#LFLAGS = -L/opt/upc/lib -lshmem -Wl,--allow-shlib-undefined -lm
#CC = icc
#CFLAGS = -I/opt/upc/include -std=c99 -O3 -w1 -g -I.

# HP SMP UPC
#LFLAGS = -L/opt/upc/lib -lm
#CC = upc
#CFLAGS = -I/opt/upc/include -O3 -upc_backend icc -I.

SRCS = 	mpp_bench.c test_all2all.c test_barrier.c test_bcast.c \
	test_pingpong.c test_reduce.c mpp_bench.h makefile README

OBJS1 = test_all2all.o mpp_bench.o
OBJS2 = test_barrier.o mpp_bench.o
OBJS3 = test_bcast.o mpp_bench.o
OBJS4 = test_pingpong.o mpp_bench.o
#OBJS4 = test_pingpong_jwf.o mpp_bench.o
OBJS5 = test_reduce.o mpp_bench.o

all:	all2all barrier bcast pingpong reduce

all2all: $(OBJS1)
	$(CC) $(CFLAGS) -o ../bin/$@ $(OBJS1) $(LFLAGS)
	mv ../bin/all2all ../bin/all2all.$(SUFFIX)

barrier: $(OBJS2)
	$(CC) $(CFLAGS) -o ../bin/$@ $(OBJS2) $(LFLAGS)
	mv ../bin/barrier ../bin/barrier.$(SUFFIX)

bcast:	$(OBJS3)
	$(CC) $(CFLAGS) -o ../bin/$@ $(OBJS3) $(LFLAGS)
	mv ../bin/bcast ../bin/bcast.$(SUFFIX)

pingpong: $(OBJS4)
	$(CC) $(CFLAGS) -o ../bin/$@ $(OBJS4) $(LFLAGS)
	mv ../bin/pingpong ../bin/pingpong.$(SUFFIX)

reduce: $(OBJS5)
	$(CC) $(CFLAGS) -o ../bin/$@ $(OBJS5) $(LFLAGS)
	mv ../bin/reduce ../bin/reduce.$(SUFFIX)

$(OBJS1): makefile mpp_bench.h

$(OBJS2): makefile mpp_bench.h

$(OBJS3): makefile mpp_bench.h

$(OBJS4): makefile mpp_bench.h

$(OBJS5): makefile mpp_bench.h

.c.o:
	$(CC) $(CFLAGS) -c $<

mpp_bench.tgz: $(SRCS)
	tar -zcvf $@ $(SRCS)

clean:
	rm -f *~ $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) 

cleanall: clean
	rm -f *.w2c.* ../bin/*
