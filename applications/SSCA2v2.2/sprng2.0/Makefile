include ../Makefile.var
LIBDIR = lib

FFXN = -DAdd__
SPRNGCFLAGS = $(CFLAGS) -DLittleEndian 

SPRNG_COMMON_DEPEND = primes_32.o primes_64.o \
			  fwrap_mpi.o cputime.o \
		      makeseed.o simple_mpi.o \
		      memory.o  communicate.o  \
			  checkid.o store.o

SPRNG_RNG_DEPEND = lcg.o lfg.o lcg64.o mlfg.o cmrg.o

all: $(SPRNG_COMMON_DEPEND) $(SPRNG_RNG_DEPEND) sprng.o
	rm -f $(LIBDIR)/libsprng.a
	$(AR) $(ARFLAGS) $(LIBDIR)/libsprng.a sprng.o \
		$(SPRNG_RNG_DEPEND) \
		$(SPRNG_COMMON_DEPEND)
	$(RANLIB) $(LIBDIR)/libsprng.a

#---------------------------------------------------------------------------
# Create files that all generators need
#---------------------------------------------------------------------------
SPRNG_COMMON_DEPEND = primes_32.o primes_64.o \
		fwrap_mpi.o  cputime.o makeseed.o  store.o\
        simple_mpi.o memory.o  communicate.o  checkid.o

sprng_common : $(SPRNG_COMMON_DEPEND)

primes_32.o : primes_32.h primes_32.c
	$(CC) -c $(SPRNGCFLAGS) primes_32.c 

primes_64.o : primes_64.h primes_64.c
	$(CC) -c $(SPRNGCFLAGS) primes_64.c 

fwrap_mpi.o : fwrap.h fwrap_mpi.c
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) fwrap_mpi.c

cputime.o : cputime.c fwrap.h
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) cputime.c

makeseed.o : makeseed.c
	$(CC) -c $(SPRNGCFLAGS) makeseed.c

simple_mpi.o : simple_mpi.c memory.h sprng_interface.h
	$(CC) -c $(SPRNGCFLAGS) simple_mpi.c

memory.o : memory.c
	$(CC) -c $(SPRNGCFLAGS) memory.c

communicate.o : communicate.c memory.h
	$(CC) -c $(SPRNGCFLAGS) communicate.c

checkid.o : checkid.c memory.h
	$(CC) -c $(SPRNGCFLAGS) checkid.c

store.o : store.c store.h
	$(CC) -c $(SPRNGCFLAGS) store.c

#---------------------------------------------------------------------------
# Build generators
#---------------------------------------------------------------------------
	
cmrg.o : sprng_interface.h cmrg.c  cmrg.h memory.h  \
		primes_64.h  multiply.h \
		store.h fwrap_.h
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) $(INLINEOPT)  cmrg.c -I.

lcg.o : sprng_interface.h lcg.c  lcg.h memory.h  \
		primes_32.h  multiply.h \
		store.h fwrap_.h
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) $(INLINEOPT)  lcg.c -I.

lcg64.o : sprng_interface.h lcg64.c  lcg64.h memory.h  \
		primes_64.h  multiply.h \
		store.h fwrap_.h
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) $(INLINEOPT)  lcg64.c -I.

lfg.o : sprng_interface.h lfg.c lfg.h memory.h  \
		multiply.h \
		store.h fwrap_.h
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) $(INLINEOPT)  lfg.c -I.

mlfg.o : sprng_interface.h mlfg.c mlfg.h int64.h memory.h  \
		multiply.h \
		store.h fwrap_.h
	$(CC) -c $(SPRNGCFLAGS) $(FFXN) $(INLINEOPT)  mlfg.c -I.

sprng.o : sprng_interface.h sprng.c memory.h  \
	multiply.h store.h fwrap_.h
	$(CC) -c $(SPRNGCFLAGS)  $(FFXN) $(INLINEOPT) sprng.c -I.

clean :
	rm -f *.o *.f *~ *.i core a.out lib/*
