QS_DIR=../queues

CC=g++
CFLAGS=-g -fopenmp
CFLAGS+= -I../greenery -L../greenery -I$(QS_DIR) -L$(QS_DIR) 
LIBS+= -lgreenery -lcorequeue #-lrt

#TODO abstract this
SOURCES=\
	MemoryDescriptor.cpp \
	SplitPhase.cpp \
	Delegate.cpp

HEADERS=\
	SplitPhase.hpp \
	GmTypes.hpp \
	MemoryDescriptor.hpp \
	Delegate.hpp

LIB_OBJ_FILES= $(SOURCES:%.cpp=%.o)

all: TestSplitPhase.exe TestDelegate.exe

TestSplitPhase.exe: TestSplitPhase.cpp $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) MemoryDescriptor.cpp SplitPhase.cpp


TestDelegate.exe: TestDelegate.cpp $(HEADERS) GlobalMemoryStub.cpp
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(SOURCES) GlobalMemoryStub.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) $< -c -o $@ $(LIBS) 

libsplitphase.a: $(LIB_OBJ_FILES) GlobalMemoryQ.o
	rm -f $@
	ar cq $@ $(LIB_OBJ_FILES) GlobalMemoryQ.o

libbprintf.a: BufferedPrinter.o
	rm -f $@
	ar cq $@ BufferedPrinter.o


clean:
	rm -f TestSplitPhase.exe TestDelegate.exe libsplitphase.a libbprintf.a ./*.o
