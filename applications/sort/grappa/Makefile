
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

SOFTXMT_HOME?=$(shell git rev-parse --show-toplevel)
# check if autodetect SOFTXMT_HOME is consistent
AUTO_HOME=$(shell git rev-parse --show-toplevel)
ifneq ($(SOFTXMT_HOME), $(AUTO_HOME))
$(warning warning: Environment variable SOFTXMT_HOME was set but doesn't match the autodetected home: env=$(SOFTXMT_HOME), auto=$(AUTO_HOME))
endif

BOOST?=/sampa/share/boost_1_50_0

#include common SoftXMT definitions
include $(SOFTXMT_HOME)/include.mk

SRUN_FLAGS?=--time=00:05:00

TARGET:= sort.exe

#default rule: just build
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(SOFTXMT_HOME)/system
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math

LDFLAGS = -g -lboost_filesystem

SOURCES = main.o
HEADERS = 

include $(SOFTXMT_HOME)/system/Makefile

sort.exe: $(SOURCES) $(HEADERS) $(SOFTXMT_HOME)/system/libSoftXMT.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) $(SOFTXMT_HOME)/system/libSoftXMT.a $(LIBRARIES) -o $@ $(LIBS)

$(SOFTXMT_HOME)/system/libSoftXMT.a: force
	cd $(SOFTXMT_HOME)/system; $(MAKE) lib

force: ;

allclean: clean
	pushd $(SOFTXMT_HOME)/system; $(MAKE) clean; popd

