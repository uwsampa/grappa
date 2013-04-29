GRAPPA_HOME?=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but does not match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

# Grappa system source directory
GRAPPA_SYS=$(GRAPPA_HOME)/system

# for building Grappa applications
include $(GRAPPA_SYS)/Makefile

# Graph500
APP_GRAPH500=../graph500

# Grappa Graph500 csr generation
APP_BFS=$(APP_GRAPH500)/grappa

# Grappa Graph500 graph tuples generation
GEN=$(APP_GRAPH500)/generator

CFLAGS += -I$(GRAPPA_SYS) -I$(APP_GRAPH500) -I$(GEN) -I$(APP_BFS)
CFLAGS += -g -Drestrict=__restrict__ -ffast-math -DGRAPH_GENERATOR_GRAPPA #-Wall

# for make_graph.h
CFLAGS += -D_GRAPPA


TARGET:= join.exe

CFLAGS += -I$(GRAPPA_SYS) 
CFLAGS += -g -Drestrict=__restrict__ #-Wall

SOURCES = $(APP_GRAPH500)/prng.o Tuple.o

join.exe: join.o DHT.hpp relation_IO.hpp Tuple.hpp $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(CFLAGS) $(LDFLAGS) join.o $(SOURCES) $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

triangles.exe: triangles.o DHT.hpp relation_IO.hpp Tuple.hpp $(GEN)/generator.a $(GRAPPA_SYS)/libGrappa.a $(SOURCES)
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(CFLAGS) $(LDFLAGS) triangles.o $(SOURCES) $(GEN)/generator.a $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

# use graph500 makefile
$(APP_GRAPH500)/prng.o:
	cd $(APP_GRAPH500); $(MAKE) CC=$(CC) prng.o  # pass on compiler

# use Grappa graph generation makefile
# pass on the DEBUG flag
$(GEN)/generator.a:
	cd $(GEN); $(MAKE) DEBUG=$(DEBUG) -f Makefile.grappa genlib

clean:
	rm -f join.o join.exe triangles.exe triangles.o
	

# XXX remove
Ref_test.exe: Ref_test.o $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) Ref_test.o $(SOURCES) $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

