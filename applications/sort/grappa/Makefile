
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.
GRAPPA_HOME?=$(shell (while [[ ! -e include.mk && "`pwd`" != '/' ]]; do cd ..; done; pwd ))
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell (while [[ ! -e include.mk && "`pwd`" != '/' ]]; do cd ..; done; pwd ))
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but doesn't match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

BOOST?=/sampa/share/boost_1_50_0

#include common Grappa definitions
include $(GRAPPA_HOME)/include.mk

SRUN_FLAGS?=--time=00:05:00

TARGET:= sort.exe

#default rule: just build
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(GRAPPA_HOME)/system
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math

LDFLAGS = -g -lboost_filesystem

SOURCES = main.o
HEADERS = 

include $(GRAPPA_HOME)/system/Makefile

sort.exe: $(SOURCES) $(HEADERS) $(GRAPPA_HOME)/system/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

$(GRAPPA_HOME)/system/libGrappa.a: force
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd

