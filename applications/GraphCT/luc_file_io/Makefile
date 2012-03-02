include ../Make.inc
PLCMD :=

all: luc_file_io.C  
	$(CXX) $(CFLAGS) -c luc_file_io.C -o luc_file_io.o
	ar r libluc_file_io.a luc_file_io.o
	ranlib libluc_file_io.a
ifeq ($(BUILD_SERVER), Yes)
	make -f Makefile.server
endif

clean:
	rm -f *.o *.a 
	make -f Makefile.server clean

