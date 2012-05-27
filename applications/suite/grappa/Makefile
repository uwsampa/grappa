
#include common SoftXMT definitions
include $(SOFTXMT_HOME)/include.mk

#SRUN_FLAGS:=--time=00:40:00

TARGET:= suite.exe

#default rule: just build
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(SOFTXMT_HOME)/system
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math

LDFLAGS = -g

SOURCES = main.o centrality.o computeGraph.o connectedComponents.o genScalData.o graphManip.o pathIsomorphism.o timer.o triangles.o
HEADERS = defs.hpp

include $(SOFTXMT_HOME)/system/Makefile

suite.exe: $(SOURCES) $(HEADERS) $(SOFTXMT_HOME)/system/libSoftXMT.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) $(SOFTXMT_HOME)/system/libSoftXMT.a $(LIBRARIES) -o $@ $(LIBS)

$(SOFTXMT_HOME)/system/libSoftXMT.a: force
	cd ../../../system; $(MAKE) lib

force: ;

allclean: clean
	pushd ../../../system; $(MAKE) clean; popd

