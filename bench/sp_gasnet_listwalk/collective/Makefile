GASNET?=/sampa/share/gasnet-rhel6
GL_ARRAY=../../../mark_scratch


#defualt gasnet library choice
MA_CONDUIT_LIB?=ibv
MA_CONDUIT_DEF?=IBV
MA_THREADING_GASNET_DEF?=PAR
MA_THREADING_GASNET_LIB?=par


CFLAGS= -Wall -O2 -std=gnu99
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(MA_CONDUIT_LIB)-conduit -DGASNET_$(MA_THREADING_GASNET_DEF) -DGASNET_CONDUIT_$(MA_CONDUIT_DEF)
CFLAGS+= -I$(GL_ARRAY)

CC=mpicc
LD=mpicc

GASNET_LIBS= -L$(GASNET)/lib -lgasnet-$(MA_CONDUIT_LIB)-$(MA_THREADING_GASNET_LIB) -libverbs $(MA_ADD_GASNET_LIBS)
LIBS= -L$(GL_ARRAY)

HEADERS=collective.h
GL_OBJ=$(GL_ARRAY)/global_memory.o $(GL_ARRAY)/global_array.o

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

test_collective: test_collective.o collective.o $(GL_OBJ)
	$(LD) -o $@ $^ $(MPI_INCLUDE) $(LIBS) $(GASNET_LIBS)

run: test_collective
	mpirun -ppn 1 -H "n01,n04" ./test_collective

clean:
	rm -f ./*.o test_collective
