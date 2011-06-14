CC=g++
CFLAGS=-g
CFLAGS+= -I../greenery -I../queues -L../greenery
LIBS+= -lgreenery #-lrt

#TODO abstract this
SOURCES=\
    ../queues/MCRingBuffer.c

HEADERS=\
  SplitPhase.hpp \
  GmTypes.hpp \
  MemoryDescriptor.hpp  

all: TestSplitPhase.exe

TestSplitPhase.exe: TestSplitPhase.cpp $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(SOURCES)


clean:
	rm -f TestSplitPhase.exe
