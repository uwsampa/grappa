GASNET?=/sampa/share/gasnet-rhel6
GL_ARRAY=../global_array_gasnet

CC=mpicxx
LD=mpicxx

#defualt gasnet library choice
MA_CONDUIT_LIB?=ibv
MA_CONDUIT_DEF?=IBV
MA_THREADING_GASNET_DEF?=PAR
MA_THREADING_GASNET_LIB?=par

CFLAGS_COMMON= -Wno-write-strings -O3 -g -fopenmp
CFLAGS= $(CFLAGS_COMMON) 
CFLAGS+= -I../global_memory
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(MA_CONDUIT_LIB)-conduit -DGASNET_$(MA_THREADING_GASNET_DEF) -DGASNET_CONDUIT_$(MA_CONDUIT_DEF)
CFLAGS+= -I$(GL_ARRAY) 

LD_FLAGS= $(CFLAGS_COMMON) 
LIB_PATHS= -L../global_memory -L$(GL_ARRAY)
LIBS= -lrt -lsplitphase

GASNET_LIB_PATHS= -L$(GASNET)/lib
GASNET_LIBS= -lgasnet-$(MA_CONDUIT_LIB)-$(MA_THREADING_GASNET_LIB) -libverbs $(MA_ADD_GASNET_LIBS)

OBJECTS=\
	getput.o	

GL_OBJECTS=\
	$(GL_ARRAY)/global_memory.o \
	$(GL_ARRAY)/global_array.o \
	$(GL_ARRAY)/msg_aggregator.o

getput.o: getput.c
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp $(HEADERS) Makefile
	$(CC) $(CFLAGS) $< -c -o $@

tree: tree.o $(OBJECTS) $(GL_OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LIB_PATHS) $(GASNET_LIB_PATHS) $(LIBS) $(GASNET_LIBS) 

pp: tree.cpp
	cpp -I$(GL_ARRAY) -I../global_memory $< 

workTest: workTest.o 
	$(LD) $(CFLAGS) $^ -o $@
	
clean:
	rm -f ./*.o ./workTest 
