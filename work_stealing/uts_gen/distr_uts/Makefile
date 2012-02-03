GASNET?=/sampa/share/gasnet-rhel6
GL_ARRAY=../../../global_array_gasnet

CXX=mpicxx
LD= mpicxx

#defualt gasnet library choice
MA_CONDUIT_LIB?=ibv
MA_CONDUIT_DEF?=IBV
MA_THREADING_GASNET_DEF?=SEQ
MA_THREADING_GASNET_LIB?=seq

ifeq ($(MA_CONDUIT_LIB), mpi)
GASLOADS=-lammpi
else
GASLOADS=-libverbs
endif

# ------------------------------------- #
# Set Random Number Generator sources:
# ------------------------------------- #

# Set the default RNG
RNG_CONTDIR=..

ifndef RNG
RNG=BRG
endif

ifeq ($(RNG), Devine) 
RNG_SRC = $(RNG_CONTDIR)/rng/devine_sha1.c
RNG_INCL= $(RNG_CONTDIR)/rng/devine_sha1.h
RNG_DEF = -DDEVINE_RNG
endif
ifeq ($(RNG), BRG)
RNG_SRC = $(RNG_CONTDIR)/rng/brg_sha1.c
RNG_INCL=  $(RNG_CONTDIR)/rng/brg_sha1.h
RNG_DEF = -DBRG_RNG
endif
ifeq ($(RNG), ALFG)
RNG_SRC = $(RNG_CONTDIR)/rng/alfg.c
RNG_INCL= $(RNG_CONTDIR)/rng/alfg.h
RNG_DEF = -DUTS_ALFG
endif
#-----------------------------------------#

CFLAGS_COMMON= -Wno-write-strings -g -fopenmp -fomit-frame-pointer
CFLAGS= $(CFLAGS_COMMON)
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(MA_CONDUIT_LIB)-conduit -DGASNET_$(MA_THREADING_GASNET_DEF) -DGASNET_CONDUIT_$(MA_CONDUIT_DEF)
CFLAGS+= -I$(GL_ARRAY)
CFLAGS+= -I.. -I../../../bench/sp_gasnet_listwalk/collective
TH_CFLAGS=-I../../../greenery 
TH_LIB=-L../../../greenery -lgreenery
LIB=-lm 

LD_FLAGS= $(CFLAGS_COMMON)

GASNET_LIB_PATHS= -L$(GASNET)/lib
GASNET_LIBS= -lgasnet-$(MA_CONDUIT_LIB)-$(MA_THREADING_GASNET_LIB) $(GASLOADS) $(MA_ADD_GASNET_LIBS)

OBJECTS=\
	balloc.o \
	getput.o \
	gasnet_cbarrier.o \
	StealQueue.o \
	uts.o 

GL_OBJECTS=\
	$(GL_ARRAY)/global_memory.o \
	$(GL_ARRAY)/global_array.o 
#	$(GL_ARRAY)/msg_aggregator.o

COLL_OBJECTS=\
	../../../bench/sp_gasnet_listwalk/collective/collective.o

%.o: %.c
	$(CXX) $(CFLAGS) $(TH_CFLAGS) $(RNG_DEF) $(RNG_INCL) $< -c -o $@

TreeGen: TreeGen.o $(OBJECTS) 
	$(CXX) $(CFLAGS) $(TH_CFLAGS) -o $@ $^ $(GL_OBJECTS) $(COLL_OBJECTS) $(RNG_SRC) $(GASNET_LIB_PATHS) $(TH_LIB) $(LIB) $(GASNET_LIBS) 

gasnet_cbarrier_test: gasnet_cbarrier_test.c gasnet_cbarrier.c
	$(CXX) $(CFLAGS) -o $@ $+ $(GASNET_LIB_PATHS) $(LIB) $(GASNET_LIBS)

run: TreeGen
	mpirun -np 6 -host "localhost" ./$< $(T1)

debug: TreeGen
	export GASNET_BACKTRACE=1
	mpirun -np 2 -host "localhost" xterm -hold -e gdb --args ./$< $(T1)

test_gcb: gasnet_cbarrier_test
	mpirun -np 3 -host "localhost" ./$< 

clean:
	rm -f ./TreeGen ./*.o ./gasnet_cbarrier
