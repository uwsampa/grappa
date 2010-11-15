SHELL = /bin/sh
.SUFFIXES: .cc .o

default: all

include ../makefile.gnu.config

LIBS = 
INCS = 

DBG = 
OPT = -O3
CXXFLAGS = -Wall -Wno-unknown-pragmas -Winline $(DBG) $(OPT)

PINFLAGS = 

OBJS  = PthreadUtil.o \
	PthreadAttr.o \
	PthreadOnce.o \
	PthreadKey.o \
	PthreadMutexAttr.o \
	PthreadMutex.o \
	PthreadCondAttr.o \
	PthreadCond.o \
	PthreadCleanup.o \
	PthreadCancel.o \
	PthreadJoin.o \
	PthreadScheduler.o \
	PthreadMalloc.o \
	PthreadSim.o \

all: mypthreadtool libmypthread.a

mypthreadtool : $(OBJS) mypthreadtool.o 
	$(CXX) $(OBJS) $@.o $(PIN_LDFLAGS) -o $@ $(PIN_LIBS) $(LIBS)

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) $(INCS) -o $@ $<

libmypthread.a : mypthread.cc
	$(CC) -c -o mypthread.o mypthread.cc
	ar ru libmypthread.a mypthread.o
	ranlib libmypthread.a
clean:
	-rm -f *.o mypthreadtool pin.log

