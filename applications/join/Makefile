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

join.exe: join.o DHT.hpp $(GRAPPA_SYS)/libGrappa.a
	$(ENV_VARIABLES) $($(MPITYPE)_LD) $(LDFLAGS) join.o $(SOURCES) $(GRAPPA_SYS)/libGrappa.a $(LIBRARIES) -o $@ $(LIBS)

clean:
	rm -f join.o join.exe
	
