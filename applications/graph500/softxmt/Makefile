
#include common SoftXMT definitions
include $(SOFTXMT_HOME)/include.mk

TARGET:= graph.exe

#default rule: just build
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(SOFTXMT_HOME)/system
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math -DGRAPH_GENERATOR_SOFTXMT
# CFLAGS += -g -pg
# CFLAGS = -g -Wall -Drestrict= -DGRAPH_GENERATOR_MPI

LDFLAGS = -g # -g -pg

# GENERATOR_SOURCES = ../generator/graph_generator.o ../generator/make_graph.o ../generator/splittable_mrg.o ../generator/utils.o
GENERATOR_SOURCES = graph_generator.o make_graph.o splittable_mrg.o utils.o
SOURCES = main.o oned_csr.o rmat.o
HEADERS = common.h oned_csr.h

include $(SOFTXMT_HOME)/system/Makefile

graph.exe: $(SOURCES) $(HEADERS) generator.a $(SOFTXMT_HOME)/system/libSoftXMT.a
	$(LD) $(LDFLAGS) $(SOURCES) generator.a $(SOFTXMT_HOME)/system/libSoftXMT.a $(LIBRARIES) -o $@

generator.a: ../generator/graph_generator.c ../generator/make_graph.c ../generator/splittable_mrg.c ../generator/utils.c
	cd ../generator; $(MAKE) -f Makefile.softxmt lib
	cd ../softxmt; cp ../generator/generator.a generator.a
	

