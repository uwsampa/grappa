
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

SHELL=/bin/sh
BENCHMARK=is
BENCHMARKU=IS

GRAPPA_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')

SRUN_FLAGS?=--time=00:15:00
	
#include common Grappa definitions
include $(GRAPPA_HOME)/include.mk

SOURCES = intsort.o randlc.o
HEADERS = randlc.hpp npb_intsort.hpp
TARGET:= intsort.exe


# default rule: just build
$(TARGET):

include $(GRAPPA_HOME)/system/Makefile

intsort.exe: $(SOURCES) $(HEADERS) $(GRAPPA_HOME)/system/libGrappa.a
	$($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

$(GRAPPA_HOME)/system/libGrappa.a: force
	echo $(GRAPPA_HOME)
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd

