
#include common SoftXMT definitions
include $(SOFTXMT_HOME)/include.mk

#default target: graph500 executable
TARGET:= graph.exe
	
#default rule: just build TARGET
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(SOFTXMT_HOME)/system
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math -DGRAPH_GENERATOR_SOFTXMT

LDFLAGS = -g

GENERATOR_SOURCES = graph_generator.o make_graph.o splittable_mrg.o utils.o
SOURCES = main.o oned_csr.o rmat.o verify.o options.o
HEADERS = common.h oned_csr.h verify.hpp

include $(SOFTXMT_HOME)/system/Makefile

graph.exe: $(SOURCES) $(HEADERS) generator.a $(SOFTXMT_HOME)/system/libSoftXMT.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) generator.a $(SOFTXMT_HOME)/system/libSoftXMT.a $(LIBRARIES) -o $@ $(LIBS)

$(SOFTXMT_HOME)/system/libSoftXMT.a: force
	cd $(SOFTXMT_HOME)/system; $(MAKE) lib

generator.a: ../generator/graph_generator.c ../generator/make_graph.c ../generator/splittable_mrg.c ../generator/utils.c
	cd ../generator; $(MAKE) -f Makefile.softxmt lib
	cd ../softxmt; cp ../generator/generator.a generator.a
	
force: ;

allclean: clean
	pushd $(SOFTXMT_HOME)/system; $(MAKE) clean; popd

otf: force
	make -f ../../../system/Makefile.tau otf

fm: force
	make -f ../../../system/Makefile.tau fm TARGET=graph.exe

