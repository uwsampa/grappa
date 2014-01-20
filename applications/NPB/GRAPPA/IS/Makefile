########################################################################
## This file is part of Grappa, a system for scaling irregular
## applications on commodity clusters. 

## Copyright (C) 2010-2014 University of Washington and Battelle
## Memorial Institute. University of Washington authorizes use of this
## Grappa software.

## Grappa is free software: you can redistribute it and/or modify it
## under the terms of the Affero General Public License as published
## by Affero, Inc., either version 1 of the License, or (at your
## option) any later version.

## Grappa is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## Affero General Public License for more details.

## You should have received a copy of the Affero General Public
## License along with this program. If not, you may obtain one from
## http://www.affero.org/oagpl.html.
########################################################################

SHELL=/bin/sh
BENCHMARK=is
BENCHMARKU=IS

GRAPPA_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')

SRUN_FLAGS?=--time=00:15:00

#include common Grappa definitions
include $(GRAPPA_HOME)/include.mk

SOURCES = intsort.o randlc.o
HEADERS = randlc.hpp npb_intsort.hpp

TARGET = intsort.exe

# default rule: just build
$(TARGET):

include $(GRAPPA_HOME)/system/Makefile

intsort.exe: $(SOURCES) $(HEADERS) $(GRAPPA_HOME)/system/libGrappa.a
	$($(MPITYPE)_LD) $(LDFLAGS) $(CFLAGS) $(SOURCES) $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

$(GRAPPA_HOME)/system/libGrappa.a: force
	echo $(GRAPPA_HOME)
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd

