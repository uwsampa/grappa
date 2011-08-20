GASNET=/sampa/share/gasnet-rhel6
GL_ARRAY=../../../mark_scratch

CFLAGS= -Wall -O2 -std=gnu99
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit -DGASNET_PAR -DGASNET_CONDUIT_IBV
CFLAGS+= -I$(GL_ARRAY)

CC=mpicc
LD=mpicc

GASNET_LIBS= -L$(GASNET)/lib -lgasnet-ibv-par -libverbs
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
