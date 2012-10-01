GRAPPA_HOME?=$(shell git rev-parse --show-toplevel)
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell git rev-parse --show-toplevel)
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but doesn't match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

#include common Grappa definitions
include $(GRAPPA_HOME)/include.mk

SRUN_FLAGS:=--time=00:05:00

TARGET:= suite.exe

#default rule: just build
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(GRAPPA_HOME)/system
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math

LDFLAGS = -g

SOURCES = main.o centrality.o computeGraph.o connectedComponents.o genScalData.o graphManip.o pathIsomorphism.o timer.o triangles.o ../compat/mersenne.o
HEADERS = defs.hpp

include $(GRAPPA_HOME)/system/Makefile

suite.exe: $(SOURCES) $(HEADERS) $(GRAPPA_HOME)/system/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

$(GRAPPA_HOME)/system/libGrappa.a: force
	cd ../../../system; $(MAKE) lib

force: ;

allclean: clean
	pushd ../../../system; $(MAKE) clean; popd

