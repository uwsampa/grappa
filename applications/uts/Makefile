# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

#
# Compile and run the Grappa implementation of UTS-mem
# uts_grappa.cpp -> uts_grappa.exe
#


#
# see Makefile.uts for modification of original uts/Makefile
#

# include common variable definitions

GRAPPA_SYS=../../system

include $(GRAPPA_SYS)/Makefile

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
RNG_OBJ = rng/devine_sha1.o
RNG_DEF = -DDEVINE_RNG
endif
ifeq ($(RNG), BRG)
RNG_SRC = rng/brg_sha1.c
RNG_INCL= rng/brg_sha1.h
RNG_OBJ = rng/brg_sha1.o
RNG_DEF = -DBRG_RNG
endif
ifeq ($(RNG), ALFG)
RNG_SRC = rng/alfg.c
RNG_INCL= rng/alfg.h
RNG_OBJ = rng/alfg.o
RNG_DEF = -DUTS_ALFG
endif
#-----------------------------------------#

CFLAGS_COMMON= -fomit-frame-pointer
CFLAGS+= $(CFLAGS_COMMON) $(RNG_DEF) -I./rng
CFLAGS+= -I$(GRAPPA_SYS)
CFLAGS+= -I.. 

# supress c char* warnings
CFLAGS+= -Wno-write-strings 

EXPERIMENTS=/sampa/home/bdmyers/experiments
CFLAGS+= -I$(EXPERIMENTS)/lib

LIB=-lm 

LDFLAGS+= $(CFLAGS_COMMON) -L$(GRAPPA_SYS)

LIBRARIES:= -lgflags -lglog -lgasnet-ibv-seq -libverbs -lrt -lboost_unit_test_framework -lGrappa

#
# set LD_LIBRARY_PATH
#
#LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib
ENV_VARIABLES+= LD_LIBRARY_PATH="\$$LD_LIBRARY_PATH:$(LD_LIBRARY_PATH)"

ENV_VARIABLES+= GASNET_BACKTRACE=1
ENV_VARIABLES+= GLOG_logtostderr=1
#ENV_VARIABLES+= GLOG_vmodule=uts_grappa=5,Aggregator=5,Incoh*=5
ENV_VARIABLES+= GLOG_v=1

uts_grappa.exe: uts_grappa.o $(RNG_OBJ) uts.o $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) \
	$($(MPITYPE)_LD) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBRARIES) -o $@ $(LIBS)

uts.o: uts.c
	$(CXX) $(CFLAGS) $(RNG_DEF) $< -c -o $@

#debug: TreeGen.exe
#	GLOG_logtostderr=1 \
#	OMPI_MCA_btl_sm_use_knem=0 \
#	LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" \
#	GASNET_BACKTRACE=1 \
#	mpirun -np $(NPROC) -H $(HOST) xterm -hold -e gdb --args ./$< $(T1) $(ARGS)

clean:
	rm -f ./*.exe ./*.o
