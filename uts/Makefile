# include common variable definitions
include ../include.mk

SOFTXMT_SYS=../system

# ------------------------------------- #
# Set Random Number Generator sources:
# ------------------------------------- #

# Set the default RNG
ifndef RNG
RNG=BRG
endif

ifeq ($(RNG), Devine) 
RNG_SRC = rng/devine_sha1.c
RNG_INCL= rng/devine_sha1.h
RNG_DEF = -DDEVINE_RNG
endif
ifeq ($(RNG), BRG)
RNG_SRC = rng/brg_sha1.c
RNG_INCL= rng/brg_sha1.h
RNG_DEF = -DBRG_RNG
endif
ifeq ($(RNG), ALFG)
RNG_SRC = rng/alfg.c
RNG_INCL= rng/alfg.h
RNG_DEF = -DUTS_ALFG
endif
#-----------------------------------------#

CFLAGS_COMMON= -fomit-frame-pointer
CFLAGS+= $(CFLAGS_COMMON)
CFLAGS+= -I$(SOFTXMT_SYS)
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
#LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib
ENV_VARIABLES+= LD_LIBRARY_PATH="\$$LD_LIBRARY_PATH:$(LD_LIBRARY_PATH)"

ENV_VARIABLES+= GASNET_BACKTRACE=1
ENV_VARIABLES+= GLOG_logtostderr=1
ENV_VARIABLES+= GLOG_v=1

#
# run vars
#
ARGS:=
NPROC:=2
HOST:=localhost
TARGET:=
PPN:=2

%.o: %.cpp
	$(CXX) $(CFLAGS) $(RNG_DEF) $< -c -o $@

uts_grappa.exe: uts_grappa.o $(RNG_SRC)
	$(LD) $(LDFLAGS) -o $@ $^  $(LIBRARIES) $(LIB) $(GASNET_LIBS) 

run: $(TARGET) $($(MPITYPE)_ENVVAR_TEMP)
	-killall $<
	$(ENV_VARIABLES) \
	$($(MPITYPE)_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- ./$< $(ARGS)

#debug: TreeGen.exe
#	GLOG_logtostderr=1 \
#	OMPI_MCA_btl_sm_use_knem=0 \
#	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" \
#	GASNET_BACKTRACE=1 \
#	mpirun -np $(NPROC) -H $(HOST) xterm -hold -e gdb --args ./$< $(T1) $(ARGS)

clean:
	rm -f ./*.exe ./*.o
