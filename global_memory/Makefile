QS_DIR=../queues

CC=mpicxx
CFLAGS=-O3 -g -fopenmp
CFLAGS+= -I../greenery -L../greenery -I$(QS_DIR) -L$(QS_DIR) 
LIBS+= -lgreenery -lcorequeue -lrt 

HOSTNAME=$(shell hostname)

GASNET=/sampa/share/gasnet-rhel6
GL_ARRAY=../mark_scratch
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit -DGASNET_PAR -DGASNET_CONDUIT_IBV
CFLAGS+= -I$(GL_ARRAY)

#TODO abstract this
SOURCES=\
	MemoryDescriptor.cpp \
	SplitPhase.cpp 

HEADERS=\
	SplitPhase.hpp \
	GmTypes.hpp \
	MemoryDescriptor.hpp 

LIB_OBJ_FILES= $(SOURCES:%.cpp=%.o) timing.o

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
