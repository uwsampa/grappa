GRAPPA_HOME?=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but does not match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

#include common Grappa definitions
include $(GRAPPA_HOME)/include.mk

#default target: graph500 executable
TARGET:= graph.exe

#default rule: just build TARGET
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(GRAPPA_HOME)/system -I.. 
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math -DGRAPH_GENERATOR_GRAPPA

LDFLAGS += -g 

# can be set to one of:
# * bfs (public tasks, barriers rather than full join)
# * bfs_steal_phases (public tasks, but full joins for each BFS level)
# * bfs_nosteal (private tasks, no stealing, full joins)
BFS = bfs

GENERATOR_SOURCES = graph_generator.o make_graph.o splittable_mrg.o utils.o
SOURCES = main.o oned_csr.o verify.o options.o $(BFS).o ../prng.o
HEADERS = common.h oned_csr.h verify.hpp

include $(GRAPPA_HOME)/system/Makefile

graph.exe: $(SOURCES) $(HEADERS) generator.a $(GRAPPA_HOME)/system/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(SOURCES) generator.a $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

$(GRAPPA_HOME)/system/libGrappa.a: force
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

generator.a: ../generator/graph_generator.c ../generator/make_graph.c ../generator/splittable_mrg.c ../generator/utils.c
	cd ../generator; $(MAKE) -f Makefile.grappa genlib
	cd ../grappa; cp ../generator/generator.a generator.a

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd

clean:
	rm -f main.o oned_csr.o verify.o options.o $(BFS).o graph.exe
	cd ../generator; $(MAKE) -f Makefile.grappa clean

otf: force
	make -f ../../../system/Makefile.tau otf

fm: force
	make -f ../../../system/Makefile.tau fm TARGET=graph.exe

