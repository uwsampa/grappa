GRAPPA_HOME?=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but does not match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

#include common Grappa definitions
include $(GRAPPA_HOME)/include.mk

#default target: graph500 executable
TARGET:= tree_search.exe

#default rule: just build TARGET
$(TARGET):

CFLAGS += -w
CFLAGS += -I$(GRAPPA_HOME)/system -I.. 
CFLAGS += -g -Wall -Drestrict=__restrict__ -ffast-math

LDFLAGS += -g 

include $(GRAPPA_HOME)/system/Makefile

$(GRAPPA_HOME)/system/libGrappa.a: force
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd







tree_search.exe tree_search.o: $(GRAPPA_HOME)/system/libGrappa.a


