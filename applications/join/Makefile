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

TARGET:= join.exe

CFLAGS += -I$(GRAPPA_SYS) 
CFLAGS += -g -Drestrict=__restrict__ #-Wall

SOURCES = 

join.exe: join.o DHT.hpp relation_IO.hpp Tuple.hpp $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) join.o $(SOURCES) $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

triangles.exe: triangles.o DHT.hpp relation_IO.hpp Tuple.hpp $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) triangles.o $(SOURCES) $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

clean:
	rm -f join.o join.exe
	

# XXX remove
Ref_test.exe: Ref_test.o $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) Ref_test.o $(SOURCES) $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

