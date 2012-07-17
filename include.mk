#############################################################################
# common variables for softxmt project
#
# many are defined with ?=, so they can be overridden at the top.
#############################################################################

SOFTXMT_HOME?=$(HOME)


#
# common across machines
#
COMMON=../common
CFLAGS+= -I$(COMMON)


CC=gcc
CXX=g++
LD=mpiCC
NONE_CC=$(CC)
NONE_CXX=$(CXX)
NONE_LD=$(LD)






# define to build on PAL cluster
# should load modules:
#   module unload pathscale openmpi
#   module load git gcc/4.6.2 openmpi
#PAL=true
ifdef PAL
BDMYERS=/pic/people/bdmyers
NELSON=/pic/people/nels707

GASNET=$(NELSON)/gasnet
HUGETLBFS=/usr
GFLAGS=$(BDMYERS)/local
GLOG=$(BDMYERS)/local
BOOST=$(NELSON)/boost
GPERFTOOLS=$(NELSON)/gperftools
VAMPIRTRACE=$(NELSON)/vampirtrace

MPITYPE=SRUN

SRUN_PARTITION=pal
SRUN_BUILD_PARTITION=pal
SRUN_HOST=--partition $(SRUN_PARTITION) --account pal  --reservation=pal_25
SRUN_RUN=salloc --exclusive $(SRUN_FLAGS) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) $($(MPITYPE)_BATCH_TEMP)
SRUN_BUILD_CMD=
SRUN_CC=$(CC)
SRUN_CXX=$(CXX)
SRUN_LD=$(LD)
SRUN_AR=$(AR)

CFLAGS+=-DHUGEPAGES_PER_MACHINE=32
endif








# some library paths
# defaults are for sampa cluster

# include this first to override system default if necessary
BOOST?=/usr
CFLAGS+= -I$(BOOST)/include
LDFLAGS+= -L$(BOOST)/lib64 -L$(BOOST)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib

# gasnet
GASNET?=/sampa/share/gasnet-1.18.2-openmpi-4kbuf-symbols
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


HUGETLBFS?=/usr
CFLAGS+= -I$(HUGETLBFS)/include
LDFLAGS+= -L$(HUGETLBFS)/lib64

GFLAGS?=/sampa/share/gflags
CFLAGS+= -I$(GFLAGS)/include
LDFLAGS+= -L$(GFLAGS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

GLOG?=/sampa/share/glog
CFLAGS+= -I$(GLOG)/include
LDFLAGS+= -L$(GLOG)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

EZLOGGER?=/sampa/share/ezlogger
CFLAGS+= -I$(EZLOGGER)/include



GPERFTOOLS?=/sampa/share/gperftools-2.0-nolibunwind
CFLAGS+= -I$(GPERFTOOLS)/include
LDFLAGS+= -L$(GPERFTOOLS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GPERFTOOLS)/lib


VAMPIRTRACE?=/sampa/share/vampirtrace
CFLAGS+= -I$(VAMPIRTRACE)/include
LDFLAGS+= -L$(VAMPIRTRACE)/lib
LD_LIBRARY_PATH:=$(VAMPIRTRACE)/lib:$(LD_LIBRARY_PATH)


MPITYPE=SRUN



#############################################################################
# configuration for running jobs with various MPI libraries
#############################################################################

#
# Slurm with srun or salloc/openmpi-mpirun
#

# create a temporary filename for environment variables
SRUN_ENVVAR_TEMP:=$(shell mktemp -utp $(PWD) .srunrc.XXXXXXXX )
SRUN_EPILOG_TEMP:=$(shell mktemp -utp $(PWD) .srunrc_epilog.XXXXXXXX )
SRUN_BATCH_TEMP:=$(shell mktemp -utp $(PWD) .sbatch.XXXXXXXX )

# command fragment to use environment variables
SRUN_EXPORT_ENV_VARIABLES=--task-prolog=$(SRUN_ENVVAR_TEMP) --task-epilog=$(SRUN_EPILOG_TEMP)

# create an environment variable file when needed
.srunrc.%:
	echo \#!/bin/bash > $@
	for i in $(ENV_VARIABLES); do echo echo export $$i >> $@; done
	chmod +x $@

.srunrc_epilog.%:
	echo \#!/bin/bash > $@
	echo 'for i in `ipcs -m | grep $(USER) | cut -d" " -f1`; do ipcrm -M $$i; done' >> $@
	chmod +x $@

SBATCH_MPIRUN_EXPORT_ENV_VARIABLES=$(patsubst %,-x %,$(patsubst DELETEME:%,,$(subst =, DELETEME:,$(ENV_VARIABLES))))
.sbatch.%:
	echo \#!/bin/bash > $@
	for i in $(ENV_VARIABLES); do echo export $$i >> $@; done
	echo mpirun $(SBATCH_MPIRUN_EXPORT_ENV_VARIABLES) -bind-to-core -report-bindings -tag-output -- $(MY_TAU_RUN) \$$* >> $@
	echo \# Clean up any leftover shared memory regions
	echo 'for i in `ipcs -m | grep $(USER) | cut -d" " -f1`; do ipcrm -M $$i; done' >> $@
	chmod +x $@

# delete when done
.INTERMEDIATE: $(SRUN_ENVVAR_TEMP) $(SRUN_EPILOG_TEMP) $(SRUN_BATCH_TEMP)

NNODE?=$(NPROC)
NTPN?=$(PPN)

SRUN_HOST?=--partition grappa
SRUN_NPROC=--nodes=$(NNODE) --ntasks-per-node=$(PPN)

SRUN_MPIRUN?=srun --resv-ports --cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit $(SRUN_FLAGS)

SRUN_CLEAN_FILES= -f .srunrc*  .sbatch*

SRUN_PARTITION?=grappa
SRUN_BUILD_PARTITION?=sampa
SRUN_STUPID_NFS_DELAY=0.5s
SRUN_BUILD_CMD?=srun -p $(SRUN_BUILD_PARTITION) --share
SRUN_CC?=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(CC)
SRUN_CXX?=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(CXX)
SRUN_LD?=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(LD)
SRUN_AR?=sleep $(SRUN_STUPID_NFS_DELAY) && $(SRUN_BUILD_CMD) $(AR)

SRUN_RUN?=$($(MPITYPE)_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- $(MY_TAU_RUN)
