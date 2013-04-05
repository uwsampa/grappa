
COMMON=../common

CFLAGS+= -I$(COMMON) -std=c++11 -Winline -Wno-inline

CC=gcc
CXX=g++
LD=mpicxx
AR=ar

MPI_CC=$(CC)
MPI_CXX=$(CXX)
MPI_LD=$(LD)
MPI_AR=$(AR)

NNODE?=1
PPN?=2

BOOST=/usr/local
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib
CFLAGS+= -I$(BOOST)/include
LDFLAGS+= -L$(BOOST)/lib

GLOG=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(GLOG)/include
LDFLAGS+= -L$(GLOG)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

GFLAGS=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(GFLAGS)/include
LDFLAGS+= -L$(GFLAGS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

GPERFTOOLS=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(GPERFTOOLS)/include
LDFLAGS+= -L$(GPERFTOOLS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GPERFTOOLS)/lib

VAMPIRTRACE?=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(VAMPIRTRACE)/include
LDFLAGS+= -L$(VAMPIRTRACE)/lib
LD_LIBRARY_PATH:=$(VAMPIRTRACE)/lib:$(LD_LIBRARY_PATH)

MPITYPE=MPI

MPI_NPROC=-n 2 #FIXME
MPI_HOST=-hosts localhost
MPI_RUN=mpirun $(MPI_HOST) $(MPI_NPROC)

GASNET_CONDUIT=mpi

GASNET_SETTINGS+= GASNET_BACKTRACE=1
GASNET_SETTINGS+= GASNET_FREEZE_SIGNAL=SIGUSR1
GASNET_SETTINGS+= GASNET_FREEZE_ON_ERROR=1
GASNET_SETTINGS+= GASNET_FREEZE=0

TEST_LIBS=-lboost_unit_test_framework


SHMMAX=512000000

# TODO: clean up LD_LIBRARY_PATH to make this work better
CFLAGS+= -Wl,-rpath,$(LD_LIBRARY_PATH),--enable-new-dtags

CFLAGS+= -DSHMMAX=$(SHMMAX)



# gasnet
GASNET=$(GRAPPA_HOME)/tools/built_deps
GASNET_CONDUIT?=mpi #values:ibv,mpi
GASNET_THREAD=seq #values:seq,par,parsync -- seq recommended

GASNET_CONDUIT_NS:=$(shell echo $(GASNET_CONDUIT)|tr -d " ")
GASNET_THREAD_NS:=$(shell echo $(GASNET_THREAD)|tr -d " ")
GASNET_LIBS+= -lgasnet-$(GASNET_CONDUIT_NS)-$(GASNET_THREAD_NS)
ifeq ($(GASNET_CONDUIT_NS),ibv)
GASNET_LIBS+= -libverbs
endif
ifeq ($(GASNET_CONDUIT_NS),mpi)
GASNET_LIBS+= -lammpi
endif
ifeq ($(GASNET_CONDUIT_NS),udp)
GASNET_LIBS+= -lamudp
endif
GASNET_FLAGS+= -DGASNET_$(shell echo $(GASNET_THREAD_NS) | tr a-z A-Z) -DGASNET_CONDUIT_$(shell echo $(GASNET_CONDUIT_NS) | tr a-z A-Z)
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(GASNET_CONDUIT_NS)-conduit
LDFLAGS+= -L$(GASNET)/lib

# fake slurm
ENV_VARIABLES+= SLURM_JOB_NUM_NODES=$(NNODE) SLURM_NNODE=$(NNODE) SLURM_NTASKS_PER_NODE=$(PPN)
