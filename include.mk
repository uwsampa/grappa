
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

#############################################################################
# common variables for grappa project
#
# many are defined with ?=, so they can be overridden at the top.
#############################################################################

# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning Environment variable GRAPPA_HOME was set but does not match the autodetected home. $(GRAPPA_HOME), $(AUTO_HOME))
endif

#
# common across machines
#
COMMON=../common
CFLAGS+= -I$(COMMON) -std=c++11 -Winline -Wno-inline

# TODO: verify that this is not a problem and remove:
CFLAGS+= -mno-red-zone

# TODO: see if we can make this apply to just our files, not user files or libraries
#CFLAGS+= -Wconversion

# TODO: clean up LD_LIBRARY_PATH to make this work better
CFLAGS+= -Wl,-rpath,$(LD_LIBRARY_PATH),--enable-new-dtags

# Enable tcmalloc by default, since we've already built its package for profiling
ifndef VALGRIND
LIBRARIES+= -ltcmalloc
endif

LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/stage/lib

# CC=gcc
# CXX=g++
# LD=mpiCC

GCC472=/sampa/share/gcc-4.7.2/rtf
CC=$(GCC472)/bin/gcc
CXX=$(GCC472)/bin/g++
LD=$(GCC472)/bin/g++ -I/usr/include/openmpi-x86_64 -pthread -m64 -L/usr/lib64/openmpi/lib -lmpi_cxx -lmpi -ldl
LD_LIBRARY_PATH:=$(GCC472)/lib64:$(LD_LIBRARY_PATH)

NONE_CC=$(CC)
NONE_CXX=$(CXX)
NONE_LD=$(LD)


# define to build on PAL cluster
# should load modules:
#   module unload pathscale openmpi
#   module load git gcc/4.6.2 openmpi

MACHINENAME:=$(shell hostname)
ifeq ($(MACHINENAME), pal.local)
PAL=true
endif

ifdef PAL
NELSON=/pic/people/nels707

#GASNET=$(NELSON)/gasnet
HUGETLBFS=/usr
BOOST=$(NELSON)/boost
GPERFTOOLS=$(NELSON)/gperftools
VAMPIRTRACE=$(NELSON)/vampirtrace

MPITYPE=SRUN

SRUN_PARTITION=pal
SRUN_BUILD_PARTITION=pal
#SRUN_HOST=--partition $(SRUN_PARTITION) --account pal  --reservation=pal_25
SRUN_HOST=--partition $(SRUN_PARTITION) --account pal $(SRUN_RESERVE) --exclude=node0196
SRUN_RUN=salloc --exclusive $(SRUN_FLAGS) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) $($(MPITYPE)_BATCH_TEMP)
SRUN_BUILD_CMD=
SRUN_CC=$(CC)
SRUN_CXX=$(CXX)
SRUN_LD=$(LD)
SRUN_AR=$(AR)

TEST_LIBS=-lboost_unit_test_framework

#CFLAGS+=-DHUGEPAGES_PER_MACHINE=32
# 32 GB of shared memory
SHMMAX=34359738368
endif

UNAME:=$(shell uname)

ifeq ($(UNAME), Darwin)
OSX=true
endif

ifdef OSX
#GASNET=/opt/grappa
#GFLAGS=/opt/grappa
#GLOG=/opt/grappa

BOOST=/usr/local
#GPERFTOOLS=/opt/grappa
#VAMPIRTRACE=/opt/grappa
MPITYPE=OPENMPI
HOST=localhost
GASNET_CONDUIT=mpi

SHMMAX=67108864
GASNET_SETTINGS+= GASNET_BACKTRACE=1
GASNET_SETTINGS+= GASNET_FREEZE_SIGNAL=SIGUSR1
GASNET_SETTINGS+= GASNET_FREEZE_ON_ERROR=1
GASNET_SETTINGS+= GASNET_FREEZE=0
#GASNET_SETTINGS+= GASNET_NETWORKDEPTH_PP=96
#GASNET_SETTINGS+= GASNET_NETWORKDEPTH_TOTAL=1024
#GASNET_SETTINGS+= GASNET_AMCREDITS_PP=48
#GASNET_SETTINGS+= GASNET_PHYSMEM_MAX=217M

PLATFORM_SPECIFIC_LIBS=
TEST_LIBS=-lboost_unit_test_framework-mt

NNODE=1
PPN?=2
endif

# check if we're running on ec2
# TODO: can we make this more robust?
ifeq ($(shell if [ -e /etc/ec2_version ]; then echo yes; fi), yes)
MPITYPE=SRUN
SHMMAX=$(shell sysctl kernel.shmmax | cut -d' ' -f3)

BOOST=/usr/local
TEST_LIBS=-lboost_unit_test_framework

SRUN_PARTITION=compute
SRUN_BUILD_PARTITION=compute
SRUN_HOST=--partition $(SRUN_PARTITION)
SRUN_BUILD_CMD=
SRUN_CC=$(CC)
SRUN_CXX=$(CXX)
SRUN_LD=$(LD)
SRUN_AR=$(AR)

CFLAGS+=-DUSE_HUGEPAGES_DEFAULT=false

# 
#EC2_UDP=true

ifdef EC2_UDP

GASNET_CONDUIT=udp
SRUN_MPIRUN?=srun --exclusive --label --kill-on-bad-exit $(SRUN_FLAGS)
SRUN_RUN=

# unfortunately running the epilog will require more configuration.
SRUN_EXPORT_ENV_VARIABLES=--task-prolog=$(SRUN_ENVVAR_TEMP)

GASNET_SETTINGS+=GASNET_SPAWNFN='C'
GASNET_SETTINGS+=GASNET_CSPAWN_CMD="$(SRUN_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- $(MY_TAU_RUN) \$$( echo '%C' | tr \\\" ' ' | cut -d' ' -f4,5,6 ) $(GARGS)"

# only defined for UDP spawners
SRUN_UDPTASKS=$(NPROC)

else

GASNET_CONDUIT=mpi
SRUN_RUN=salloc --exclusive $(SRUN_FLAGS) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) $($(MPITYPE)_BATCH_TEMP)

endif

endif

PLATFORM_SPECIFIC_LIBS?=-lrt
TEST_LIBS?=-lboost_unit_test_framework

# (for Sampa cluster) 12 GB
SHMMAX?=12884901888

# some library paths
# defaults are for sampa cluster

# include this first to override system default if necessary
BOOST?=/sampa/share/gcc-4.7.2/src/boost_1_51_0
CFLAGS+= -I$(BOOST)/boost
LDFLAGS+= -L$(BOOST)/stage/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/stage/lib

ifdef GASNET_TRACING
GASNET:=/sampa/share/gasnet-1.18.2-tracing
GASNET_SETTINGS+=GASNET_TRACEFILE="/scratch/gasnet_trace_%"
GASNET_SETTINGS+=GASNET_TRACEMASK=I
#GASNET_SETTINGS+=GASNET_TRACENODES=2-4
endif

# gasnet
GASNET=$(GRAPPA_HOME)/tools/built_deps
GASNET_CONDUIT?=ibv #values:ibv,mpi
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


HUGETLBFS?=/usr
CFLAGS+= -I$(HUGETLBFS)/include
LDFLAGS+= -L$(HUGETLBFS)/lib64

GFLAGS=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(GFLAGS)/include
LDFLAGS+= -L$(GFLAGS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

GLOG=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(GLOG)/include
LDFLAGS+= -L$(GLOG)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

GPERFTOOLS=$(GRAPPA_HOME)/tools/built_deps
#GPERFTOOLS?=/sampa/share/gperftools-2.0-nolibunwind
CFLAGS+= -I$(GPERFTOOLS)/include
LDFLAGS+= -L$(GPERFTOOLS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GPERFTOOLS)/lib


VAMPIRTRACE?=$(GRAPPA_HOME)/tools/built_deps
CFLAGS+= -I$(VAMPIRTRACE)/include
LDFLAGS+= -L$(VAMPIRTRACE)/lib
LD_LIBRARY_PATH:=$(VAMPIRTRACE)/lib:$(LD_LIBRARY_PATH)

VALGRIND_PATH?=/sampa/share/valgrind-3.8.1-cluster
CFLAGS+= -I$(VAMPIRTRACE)/include
LDFLAGS+= -L$(VAMPIRTRACE)/lib/valgrind
LD_LIBRARY_PATH:=$(VAMPIRTRACE)/lib/valgrind:$(LD_LIBRARY_PATH)


MPITYPE?=SRUN

CFLAGS+= -DSHMMAX=$(SHMMAX)


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
SRUN_EXPORT_ENV_VARIABLES?=--task-prolog=$(SRUN_ENVVAR_TEMP) --task-epilog=$(SRUN_EPILOG_TEMP)

# create an environment variable file when needed
.srunrc.%:
	@echo \#!/bin/bash > $@
	@for i in $(ENV_VARIABLES); do echo echo export $$i >> $@; done
	@echo '# Clean up any leftover shared memory regions' >> $@
	@echo 'for i in `ipcs -m | grep $(USER) | cut -d" " -f1 | grep -v 0x00000000`; do ipcrm -M $$i; done' >> $@
	@chmod +x $@

.srunrc_epilog.%:
	@echo \#!/bin/bash > $@
	@echo 'for i in `ipcs -m | grep $(USER) | cut -d" " -f1 | grep -v 0x00000000`; do ipcrm -M $$i; done' >> $@
	@chmod +x $@

# set to force libs to be recopied to scratch disks
ifdef FORCE_COPY_LIBS 
SBATCH_FORCE_COPY_LIBS=-f
endif

# directory for scratch storage of libs
SBATCH_SCRATCH_DIR=/scratch/$(USER)/libs

SBATCH_MPIRUN_EXPORT_ENV_VARIABLES=$(patsubst %,-x %,$(patsubst DELETEME:%,,$(subst =, DELETEME:,$(ENV_VARIABLES))))
.sbatch.%:
	@echo '#!/bin/bash' > $@
	@for i in $(ENV_VARIABLES); do echo "export $$i" >> $@; done
ifdef PAL	
	@echo '# Make scratch directory'  >> $@
	@echo 'mkdir -p $(SBATCH_SCRATCH_DIR)' >> $@
	@echo 'srun --ntasks-per-node=1 --ntasks=$(NNODE) mkdir -p $(SBATCH_SCRATCH_DIR)' >> $@
#	@echo 'srun bash -c "hostname; ls -ld $(SBATCH_SCRATCH_DIR)"' >> $@
	@echo '# Copy libraries to scratch directory' >> $@
	@echo 'LIBS_TO_COPY=$$( ldd $$1 | egrep -v linux-vdso\.so\|ld-linux\|"> /lib64" | sed "s/.*> \(.*\) (.*/\\1/" )' >> $@
	@echo 'for i in $$LIBS_TO_COPY; do sbcast $(SBATCH_FORCE_COPY_LIBS) $${i} $(SBATCH_SCRATCH_DIR)/$${i/*\//}; done' >> $@
	@echo 'for i in $$LIBS_TO_COPY; do cp $${i} $(SBATCH_SCRATCH_DIR)/$${i/*\//}; done' >> $@
	@echo 'export LD_LIBRARY_PATH=$(SBATCH_SCRATCH_DIR)' >> $@
	@echo '# Reformat command and (force) copy binary to scratch directory' >> $@
	@echo 'export CMD=$${*/.\//$(SBATCH_SCRATCH_DIR)\/}' >> $@
	@echo 'export BINARY=$${1/.\//$(SBATCH_SCRATCH_DIR)\/}' >> $@
	@echo 'sbcast -f $(SBATCH_FORCE_COPY_LIBS) $$1 $$BINARY' >> $@
	@echo 'cp -f $(SBATCH_FORCE_COPY_LIBS) $$1 $$BINARY' >> $@
	@echo '# Copy mpirun to scratch directory' >> $@
	@echo 'export SYSTEM_MPIRUN=`which mpirun`' >> $@
	@echo 'export MPIRUN=$${SYSTEM_MPIRUN/*\//$(SBATCH_SCRATCH_DIR)\/}' >> $@
	@echo 'sbcast $(SBATCH_FORCE_COPY_LIBS) $$SYSTEM_MPIRUN $$MPIRUN' >> $@
	@echo 'cp $(SBATCH_FORCE_COPY_LIBS) $$SYSTEM_MPIRUN $$MPIRUN' >> $@
#	@echo 'echo $$CMD' >> $@
#	@echo 'ldd $$1' >> $@
#	@echo 'ldd `which mpirun`' >> $@
#	@echo mpirun $(SBATCH_MPIRUN_EXPORT_ENV_VARIABLES) -bind-to-core -report-bindings -tag-output -- $(MY_TAU_RUN) \$$* >> $@
	@echo '# Run!' >> $@
	@echo '$(SBATCH_SCRATCH_DIR)/mpirun $(SBATCH_MPIRUN_EXPORT_ENV_VARIABLES) -bind-to-core -tag-output -- $(MY_TAU_RUN) $$CMD' >> $@
else
	@echo '# Run!' >> $@
	@echo 'mpirun -npernode 1 bash -c "ipcs -m | grep $(USER) | cut -d\  -f1 | xargs -n1 -r ipcrm -M"' >> $@
	@echo 'mpirun $(SBATCH_MPIRUN_EXPORT_ENV_VARIABLES) -bind-to-core -tag-output -- $(MY_TAU_RUN) $$*' >> $@
	@echo 'mpirun -npernode 1 bash -c "ipcs -m | grep $(USER) | cut -d\  -f1 | xargs -n1 -r ipcrm -M"' >> $@
endif
	@echo '# Clean up any leftover shared memory regions' >> $@
	@echo 'for i in `ipcs -m | grep $(USER) | cut -d" " -f1`; do ipcrm -M $$i; done' >> $@
	@chmod +x $@

# delete when done
.INTERMEDIATE: $(SRUN_ENVVAR_TEMP) $(SRUN_EPILOG_TEMP) $(SRUN_BATCH_TEMP)

NNODE?=2
PPN?=1
NTPN?=$(PPN)
NPROC=$(shell echo $$(( $(NNODE)*$(PPN) )) )

SRUN_HOST?=--partition $(SRUN_PARTITION)
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

#SRUN_RUN?=$($(MPITYPE)_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- $(MY_TAU_RUN)

# OpenMPI
OPENMPI_NPROC= --n $(NPROC) --npernode $(PPN)
#OPENMPI_RUN=mpirun $($(MPI_TYPE)_MPIRUN) 
OPENMPI_BUILD_CMD?=
OPENMPI_CXX=mpic++
OPENMPI_CC=mpicc
OPENMPI_LD=mpic++
OPENMPI_AR=ar
OPENMPI_CLEAN_FILE= *.btr
OPENMPI_EXPORT_ENV_VARIABLES=$(patsubst %,-x %,$(patsubst DELETEME:%,,$(subst =, DELETEME:,$(ENV_VARIABLES))))
OPENMPI_HOST=-H $(HOST)
#OPENMPI_NPROC=-np $(NPROC)
OPENMPI_MPIRUN=mpirun

##########################
# All MPI types...
$(MPITYPE)_RUN?=$($(MPITYPE)_MPIRUN) $($(MPITYPE)_EXPORT_ENV_VARIABLES) $($(MPITYPE)_HOST) $($(MPITYPE)_NPROC) -- $(MY_TAU_RUN)
