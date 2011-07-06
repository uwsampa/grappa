QS_DIR=../queues

CC=g++
CFLAGS=-O3 -g -fopenmp
CFLAGS+= -I../greenery -L../greenery -I$(QS_DIR) -L$(QS_DIR) 
LIBS+= -lgreenery -lcorequeue -lrt 

# for GA
CFLAGS+= -I../tools/ga-5-0-2/ga-n01/include -L../tools/ga-5-0-2/ga-n01/lib
LIBS+= -lga++ -lga -lgfortran -libverbs

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

libsplitphase.a: $(LIB_OBJ_FILES) 
	rm -f $@
	ar cq $@ $(LIB_OBJ_FILES) 

libbprintf.a: BufferedPrinter.o
	rm -f $@
	ar cq $@ BufferedPrinter.o


clean:
	rm -f TestSplitPhase.exe TestDelegate.exe libsplitphase.a libbprintf.a ./*.o
