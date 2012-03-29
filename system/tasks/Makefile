# include common variable definitions
include ../../include.mk

SOFTXMT_SYS=..


CFLAGS_COMMON= -fopenmp -fomit-frame-pointer
CFLAGS+= $(CFLAGS_COMMON)
CFLAGS+= -I$(GL_ARRAY) -I$(SOFTXMT_SYS)
CFLAGS+= -I..

ifdef DEBUG
CFLAGS+= -O1 -g
else
CFLAGS+= -O3
endif

# supress c char* warnings
CFLAGS+= -Wno-write-strings 

CFLAGS+= -I$(EXPERIMENTS)/lib

CFLAGS+= -DGASNET_SEQ -DGASNET_CONDUIT_IBV
CXXFLAGS=$(CFLAGS)

LIB=-lm 

LDFLAGS+= $(CFLAGS_COMMON) -L$(SOFTXMT_SYS)

LIBRARIES:= -lgflags -lglog -lgasnet-ibv-seq -libverbs -lrt -lboost_unit_test_framework -lSoftXMT

#
# set LD_LIBRARY_PATH
#
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

objects: gasnet_cbarrier.o StealQueue.o Thread.o Task.o Scheduler.o

clean:
	rm -f ./*.o
