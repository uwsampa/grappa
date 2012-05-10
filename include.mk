#############################################################################
# common variables for softxmt project
#############################################################################

SOFTXMT_HOME?=$(HOME)

#
# common across machines
#
COMMON=../common
CFLAGS+= -I$(COMMON)

#GREENERY=../greenery
#CFLAGS+= -I$(GREENERY)
#LDFLAGS+= -L$(GREENERY)

#
# different for specific machines
#

# #
# # for janus03? at HPC Advisory Council
# #

# CC=gcc44
# CXX=g++44
# #CXX=mpic++
# LD=mpic++

# #CFLAGS+= -std=c++0x

# # some library paths
# GASNET=/home/jnelson/gasnet18-mellanox-default
# CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit
# LDFLAGS+= -L$(GASNET)/lib

# HUGETLBFS=/home/jnelson/libhugetlbfs-2.12
# CFLAGS+= -I$(HUGETLBFS)/include
# LDFLAGS+= -L$(HUGETLBFS)/lib

# GFLAGS=/home/jnelson/gflags-install
# CFLAGS+= -I$(GFLAGS)/include
# LDFLAGS+= -L$(GFLAGS)/lib
# LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

# GLOG=/home/jnelson/glog-install
# CFLAGS+= -I$(GLOG)/include
# LDFLAGS+= -L$(GLOG)/lib
# LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

# BOOST=/home/jnelson/boost-install
# CFLAGS+= -I$(BOOST)/include
# LDFLAGS+= -L$(BOOST)/lib
# LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib

# MPITYPE=OPENMPI
# MPIRUN=mpirun


#
# for n01, n02, n04 
#

CC=gcc
CXX=g++
LD=mpiCC

# some library paths

# gasnet
GASNET=/sampa/share/gasnet18-rhel6-openmpi
GASNET_CONDUIT=ibv #values:ibv,mpi
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
GASNET_FLAGS:= -DGASNET_$(shell echo $(GASNET_THREAD_NS) | tr a-z A-Z) -DGASNET_CONDUIT_$(shell echo $(GASNET_CONDUIT_NS) | tr a-z A-Z)
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/$(GASNET_CONDUIT_NS)-conduit
LDFLAGS+= -L$(GASNET)/lib


HUGETLBFS=/usr
CFLAGS+= -I$(HUGETLBFS)/include
LDFLAGS+= -L$(HUGETLBFS)/lib64

GFLAGS=/sampa/share/gflags
CFLAGS+= -I$(GFLAGS)/include
LDFLAGS+= -L$(GFLAGS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

GLOG=/sampa/share/glog
CFLAGS+= -I$(GLOG)/include
LDFLAGS+= -L$(GLOG)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

EZLOGGER=/sampa/share/ezlogger
CFLAGS+= -I$(EZLOGGER)/include

BOOST=/usr
CFLAGS+= -I$(BOOST)/include
LDFLAGS+= -L$(BOOST)/lib64
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib

GPERFTOOLS=/sampa/share/gperftools-2.0
CFLAGS+= -I$(GPERFTOOLS)/include
LDFLAGS+= -L$(GPERFTOOLS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GPERFTOOLS)/lib

MPITYPE=SRUN



#############################################################################
# configuration for running jobs with various MPI libraries
#############################################################################

#
# OpenMPI with mpirun
#

# how do we export environment variables
OPENMPI_EXPORT_ENV_VARIABLES=$(patsubst %,-x %,$(patsubst DELETEME:%,,$(subst =, DELETEME:,$(ENV_VARIABLES))))

OPENMPI_HOST=-H $(HOST)
OPENMPI_NPROC=-np $(NPROC)

OPENMPI_MPIRUN=mpirun

OPENMPI_CLEAN_FILE= *.btr

OPENMPI_CC=$(CC)
OPENMPI_CXX=$(CXX)
OPENMPI_LD=$(LD)
OPENMPI_AR=$(AR)

#
# Slurm with srun
#

# create a temporary filename for environment variables
SRUN_ENVVAR_TEMP:=$(shell mktemp --dry-run --tmpdir=$(PWD) .srunrc.XXXXXXXX )
SRUN_EPILOG_TEMP:=$(shell mktemp --dry-run --tmpdir=$(PWD) .srunrc_epilog.XXXXXXXX )

# command fragment to use environment variables
SRUN_EXPORT_ENV_VARIABLES=--task-prolog=$(SRUN_ENVVAR_TEMP) --task-epilog=$(SRUN_EPILOG_TEMP)

# create an environment variable file when needed
.srunrc.%:
	echo \#!/bin/bash > $@
	for i in $(ENV_VARIABLES); do echo echo export $$i >> $@; done
	chmod +x $@

.srunrc_epilog.%:
	echo \#!/bin/bash > $@
	echo 'for i in `ipcs -m | head -n-1 | tail -n+4 | sort | cut -d" " -f1`; do ipcrm -M $$i; done' >> $@
	chmod +x $@

# delete when done
.INTERMEDIATE: $(SRUN_ENVVAR_TEMP) $(SRUN_EPILOG_TEMP)

NNODE?=$(NPROC)
PPN?=1

SRUN_HOST=--partition softxmt
SRUN_NPROC=--nodes=$(NNODE) --ntasks-per-node=$(PPN)

SRUN_MPIRUN=srun --resv-ports --cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit $(SRUN_FLAGS)

SRUN_CLEAN_FILES= .srunrc.* 

SRUN_PARTITION=softxmt
SRUN_STUPID_NFS_DELAY=0.5s
SRUN_BUILD_CMD=srun -p $(SRUN_PARTITION) --share
SRUN_CC=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(CC)
SRUN_CXX=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(CXX)
SRUN_LD=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(LD)
SRUN_AR=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(AR)

#
# QLogic MPI
#

# create a temporary filename for environment variables
QLOGIC_ENVVAR_TEMP:=$(shell mktemp --dry-run --tmpdir=. .mpirunrc.XXXXXXXX )
# command fragment to use environment variables
QLOGIC_EXPORT_ENV_VARIABLES=-rcfile $(QLOGIC_ENVVAR_TEMP)

# create an environment variable file when needed
.mpirunrc.%:
	echo \#!/bin/bash > $@
	echo "if [ -e ~/.mpirunrc ]; then . ~/.mpirunrc; fi" >> $@
	for i in $(ENV_VARIABLES); do echo export $$i >> $@; done

# delete when done
.INTERMEDIATE: $(QLOGIC_ENVVAR_TEMP)

QLOGIC_HOST=-H $(HOST)
QLOGIC_NPROC=-np $(NPROC)

QLOGIC_MPIRUN=mpirun -l

QLOGIC_CLEAN_FILES= *.btr .mpirunrc.* 

QLOGIC_CC=$(CC)
QLOGIC_CXX=$(CXX)
QLOGIC_LD=$(LD)
QLOGIC_AR=$(AR)
