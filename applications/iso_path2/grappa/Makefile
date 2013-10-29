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

ifdef PAL
CFLAGS += -DUSE_MPI3_COLLECTIVES
endif

ifdef SMALL_GRAPH
CFLAGS += -DSMALL_GRAPH
endif

LDFLAGS += -g 

# can be set to one of:
# * bfs (public tasks, barriers rather than full join)
# * bfs_steal_phases (public tasks, but full joins for each BFS level)
# * bfs_nosteal (private tasks, no stealing, full joins)
BFS = bfs_local

GENERATOR_SOURCES = graph_generator.o make_graph.o splittable_mrg.o utils.o
SOURCES = graph.o oned_csr.o verify.o options.o $(BFS).o ../prng.o
HEADERS = common.h oned_csr.h verify.hpp

include $(GRAPPA_HOME)/system/Makefile

graph.exe: $(SOURCES) main.o $(HEADERS) generator.a $(GRAPPA_HOME)/system/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(CFLAGS) $(SOURCES) main.o generator.a $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

graph_new.exe: $(SOURCES) main_new.o cc_new.o $(HEADERS) generator.a $(GRAPPA_HOME)/system/libGrappa.a bfs_local_adj.o
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(CFLAGS) $(SOURCES) bfs_local_adj.o main_new.o cc_new.o generator.a $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

iso_path.exe: $(SOURCES) iso_path.o cc_new.o $(HEADERS) generator.a $(GRAPPA_HOME)/system/libGrappa.a bfs_local_adj.o
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) $(CFLAGS) $(SOURCES) bfs_local_adj.o iso_path.o cc_new.o generator.a $(GRAPPA_HOME)/system/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

$(GRAPPA_HOME)/system/libGrappa.a: force
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

generator.a: ../generator/graph_generator.c ../generator/make_graph.c ../generator/splittable_mrg.c ../generator/utils.c
	cd ../generator; $(MAKE) -f Makefile.grappa genlib
	cd ../grappa; cp ../generator/generator.a generator.a

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd

clean:
	rm -f $(SOURCES) main.o main_new.o bfs_*.o graph.exe iso_path.exe graph_new.exe generator.a
	cd ../generator; $(MAKE) -f Makefile.grappa clean

otf: force
	make -f ../../../system/Makefile.tau otf

fm: force
	make -f ../../../system/Makefile.tau fm TARGET=graph.exe

run graph_new: graph_new.exe
	../../../bin/grappa_srun.rb --nnode=2 --ppn=2 --freeze-on-error -- graph_new.exe --num_starting_workers=512 -- -s 10

run iso_path: iso_path.exe
	../../../bin/grappa_srun.rb --nnode=2 --ppn=2 --freeze-on-error -- iso_path.exe --num_starting_workers=512 -- -s 4