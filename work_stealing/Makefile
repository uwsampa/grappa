GASNET?=/sampa/share/gasnet-rhel6
GL_ARRAY=../global_array_gasnet

CC=mpicxx
LD=mpicxx

#defualt gasnet library choice
MA_CONDUIT_LIB?=ibv
MA_CONDUIT_DEF?=IBV
MA_THREADING_GASNET_DEF?=PAR
MA_THREADING_GASNET_LIB?=par

COLLECTIVE_DIR=../bench/sp_gasnet_listwalk/collective

CFLAGS_COMMON= -Wno-write-strings -O3 -g -fopenmp
CFLAGS= $(CFLAGS_COMMON) 
CFLAGS+= -I../global_memory
CFLAGS+= -I./$(COLLECTIVE_DIR)
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(MA_CONDUIT_LIB)-conduit -DGASNET_$(MA_THREADING_GASNET_DEF) -DGASNET_CONDUIT_$(MA_CONDUIT_DEF)
CFLAGS+= -I$(GL_ARRAY) 

LD_FLAGS= $(CFLAGS_COMMON) 
LIB_PATHS= -L../global_memory -L$(GL_ARRAY)
LIBS= -lrt -lsplitphase

GASNET_LIB_PATHS= -L$(GASNET)/lib
GASNET_LIBS= -lgasnet-$(MA_CONDUIT_LIB)-$(MA_THREADING_GASNET_LIB) -libverbs $(MA_ADD_GASNET_LIBS)

THREAD_DIR=../greenery
TH_CFLAGS= -I$(THREAD_DIR)
TH_LDFLAGS= -L$(THREAD_DIR) -lgreenery

OBJECTS=\
	getput.o \
	StealQueue.o

GL_OBJECTS=\
	$(GL_ARRAY)/global_memory.o \
	$(GL_ARRAY)/global_array.o \
	$(GL_ARRAY)/msg_aggregator.o \
	$(COLLECTIVE_DIR)/collective.o	

getput.o: getput.c
	$(CC) $(CFLAGS) $< -c -o $@

StealQueue.o: StealQueue.c
	$(CC) $(CFLAGS) $< -c -o $@

%.o: %.cpp $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(TH_CFLAGS) $< -c -o $@

tree: tree.o $(OBJECTS) $(GL_OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LIB_PATHS) $(GASNET_LIB_PATHS) $(LIBS) $(GASNET_LIBS) 

pp: tree.cpp
	cpp -I$(GL_ARRAY) -I../global_memory $< 

treeNew: treeNew.o $(OBJECTS) $(GL_OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LIB_PATHS) $(GASNET_LIB_PATHS) $(LIBS) $(GASNET_LIBS) $(TH_LDFLAGS)

testLocal: testLocal.o $(OBJECTS) $(GL_OBJECTS)
	$(LD) $(LD_FLAGS) -o $@ $^ $(LIB_PATHS) $(GASNET_LIB_PATHS) $(LIBS) $(GASNET_LIBS) $(TH_LDFLAGS)

workTest: workTest.o 
	$(LD) $(CFLAGS) $^ -o $@

run: treeNew	
	mpirun -np 2 -H "n01" hugectl --heap -- ./treeNew -n 1000 -i 2 -c 2 -t 2

clean:
	rm -f ./*.o ./workTest ./tree ./treeNew
