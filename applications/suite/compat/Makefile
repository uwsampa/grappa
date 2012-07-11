include ../Make.inc

all:	libxmt-ops.a

libluc_file_io_compat.a:	libluc_file_io_compat.a(luc_file_io.o)
libluc_file_io_compat.a(luc_file_io.o):	luc_file_io.o
luc_file_io.o:	luc_file_io.c

libxmt-ops.a:	libxmt-ops.a(xmt-ops.o)
libxmt-ops.a(xmt-ops.o):	xmt-ops.o mersenne.o
xmt-ops.o:	xmt-ops.c
mersenne.o: mersenne.c mersenne.h

clean:
	rm -f luc_file_io.o libluc_file_io_compat.a xmt-ops.o libxmt-ops.a
