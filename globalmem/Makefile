
CXX=mpic++

CXXFLAGS+= -O2 -g -fopenmp -std=gnu++0x -Wno-write-strings
#CXXFLAGS+= -O2 -g

#LIBS+= -lrt

HEADERS=\
	MPIReceiveWorker.hpp \


TARGETS=\
	mpitest.exe \
	ga.exe


CXXFLAGS+= -I/opt/ga/include -L/opt/ga/lib
LIBS= -lga++ -lga -framework Accelerate

run: $(TARGETS)
	mpiexec -np 1 ./$<

%.dis: %.exe
	objdump -S ./$< > $@

#%.o: %.c $(HEADERS) Makefile
#	$(CC) $(CFLAGS) $< -c -o $@ 

%.exe: %.cpp $(HEADERS) $(OBJECTS) Makefile
	$(CXX) $(CXXFLAGS) $< -o $@ $(OBJECTS) $(LIBS)

clean: 
	rm -f $(TARGETS)

.PHONY: run

