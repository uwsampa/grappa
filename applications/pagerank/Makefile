GRAPPA_HOME?=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but does not match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

# Grappa system source directory
GRAPPA_SYS=$(GRAPPA_HOME)/system

# Graph500
APP_GRAPH500=../graph500

# Grappa Graph500 csr generation
APP_BFS=$(APP_GRAPH500)/grappa

# Grappa Graph500 graph tuples generation
GEN=$(APP_GRAPH500)/generator

#ENV_VARIABLES+= GLOG_vmodule=pagerank*=5,spmv*=5,GlobalCompletionEvent*=5

# for building Grappa applications
include $(GRAPPA_SYS)/Makefile

TARGET:= spmv_mult.exe

CFLAGS += -I$(GRAPPA_SYS) -I$(APP_GRAPH500) -I$(GEN) -I$(APP_BFS)
CFLAGS += -g -Drestrict=__restrict__ -ffast-math -DGRAPH_GENERATOR_GRAPPA #-Wall

# for make_graph.h
CFLAGS += -D_GRAPPA

SOURCES = spmv_mult.o $(APP_BFS)/oned_csr.o $(APP_GRAPH500)/prng.o


mult.exe: mult_main.o $(SOURCES) $(HEADERS) $(GEN)/generator.a $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) mult_main.o $(SOURCES) $(GEN)/generator.a $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

pagerank.exe: pagerank.o $(SOURCES) $(HEADERS) $(GEN)/generator.a $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) pagerank.o $(SOURCES) $(GEN)/generator.a $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

# use graph500 makefile
$(APP_GRAPH500)/prng.o:
	cd $(APP_GRAPH500); $(MAKE) prng.o

# use Grappa graph generation makefile
$(GEN)/generator.a:
	cd $(GEN); $(MAKE) -f Makefile.grappa genlib

clean:
	rm -f spmv_mult.o mult_main.o mult.exe
	
