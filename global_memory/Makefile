CC=g++
CFLAGS=-g -fopenmp
CFLAGS+= -I../greenery -I../queues -L../greenery
LIBS+= -lgreenery #-lrt

#TODO abstract this
SOURCES=\
	../queues/MCRingBuffer.c \
	MemoryDescriptor.cpp

HEADERS=\
	SplitPhase.hpp \
	GmTypes.hpp \
	MemoryDescriptor.hpp  

all: TestSplitPhase.exe TestDelegate.exe

TestSplitPhase.exe: TestSplitPhase.cpp $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(SOURCES)

TestDelegate.exe: TestDelegate.cpp $(HEADERS) GlobalMemoryStub.cpp
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(SOURCES) GlobalMemoryStub.cpp

clean:
	rm -f TestSplitPhase.exe TestDelegate.exe
