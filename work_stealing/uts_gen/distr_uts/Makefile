# include common variable definitions
include ../../../include.mk

GL_ARRAY=../../../global_array_gasnet
SOFTXMT_SYS=../../../system

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

CFLAGS_COMMON= -fopenmp -fomit-frame-pointer
CFLAGS+= $(CFLAGS_COMMON)
CFLAGS+= -I$(GL_ARRAY) -I$(SOFTXMT_SYS)
CFLAGS+= -I..

ifdef DEBUG
CFLAGS+= -O1 -g
else
CFLAGS+= -O3
endif

# supress c char* warnings
CFLAGS+= -Wno-write-strings 

EXPERIMENTS=/sampa/home/bdmyers/experiments
CFLAGS+= -I$(EXPERIMENTS)/lib

CFLAGS+= -DGASNET_SEQ -DGASNET_CONDUIT_IBV


LIB=-lm 

LDFLAGS+= $(CFLAGS_COMMON) -L$(SOFTXMT_SYS)

LIBRARIES:= -lgflags -lglog -lgasnet-ibv-seq -libverbs -lrt -lboost_unit_test_framework -lSoftXMT

#
# set LD_LIBRARY_PATH
#
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

#
# run vars
#
ARGS:=
NPROC:=2
HOST:=localhost

OBJECTS=\
	balloc.o \
	getput.o \
	gasnet_cbarrier.o \
	StealQueue.o \
	uts.o 

GL_OBJECTS=\
	$(GL_ARRAY)/global_memory.o \
	$(GL_ARRAY)/global_array.o 

%.o: %.c
	$(CXX) $(CFLAGS) $(RNG_DEF) $(RNG_INCL) $< -c -o $@

TreeGen.exe: TreeGen.o $(OBJECTS) 
	$(LD) $(LDFLAGS) -o $@ $^ $(GL_OBJECTS) $(RNG_SRC) $(LIBRARIES)  $(LIB) $(GASNET_LIBS) 

gasnet_cbarrier_test: gasnet_cbarrier_test.c gasnet_cbarrier.c
	$(CXX) $(CFLAGS) -o $@ $+ $(GASNET_LIB_PATHS) $(LIB) $(GASNET_LIBS)

run: TreeGen.exe
	GLOG_logtostderr=1 \
	OMPI_MCA_btl_sm_use_knem=0 \
	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" \
	$(MPIRUN) -np $(NPROC) -H $(HOST) ./$< $(T1) $(ARGS)

debug: TreeGen.exe
	export GASNET_BACKTRACE=1
	mpirun -np 2 -host "localhost" xterm -hold -e gdb --args ./$< $(T1)

test_gcb: gasnet_cbarrier_test
	mpirun -np 3 -host "localhost" ./$< 

clean:
	rm -f ./*.exe ./*.o
