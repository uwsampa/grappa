
CXX=mpic++

CXXFLAGS+= -O2 -g -fopenmp -std=gnu++0x -Wno-write-strings --save-temps
#CXXFLAGS+= -O2 -g

#LIBS+= -lrt

HEADERS=\
	MPIWorker.hpp \
	MPIGlobalArray.hpp \
	GlobalArray.hpp \


TARGETS=\
	GlobalArray_test.exe \
	MPIGlobalArray_test.exe \
	MPIWorker_test.exe \
	ga.exe


CXXFLAGS+= -I/opt/ga/include -L/opt/ga/lib
LIBS= -lga++ -lga 

run: $(TARGETS)
	mpiexec -np 4 ./$<

%.dis: %.exe
	objdump -S ./$< > $@

#%.o: %.c $(HEADERS) Makefile
#	$(CC) $(CFLAGS) $< -c -o $@ 

%.exe: %.cpp $(HEADERS) $(OBJECTS) Makefile
	$(CXX) $(CXXFLAGS) $< -o $@ $(OBJECTS) $(LIBS)

clean: 
	rm -f $(TARGETS)

.PHONY: run

