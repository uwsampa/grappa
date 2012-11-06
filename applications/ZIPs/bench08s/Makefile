##########################################################################
#
#   Benchmark 8 - Makefile
#
#       make  - will provide the small dense matrix benchmark
#
#       make bench8p  - will provide the parallel version of the 
#	                 small dense matrix benchmark
#
#       make bench8s  - will provide the large sparse matrix benchmark
#
#       make clean  - will remove all *.o  *.l and  *.m  files
#
##########################################################################
#
#  CRAY Definitions:
#    -Wf"-em" -> enable source listing
#    -ZV -> Vectorization enhancement but no autotasking (parallelization)
#
##########################################################################

CC         	= cc
#CFLAGS     	= -Oscalar3,vector3 -h report=svf
CFLAGS     	= -Oscalar3,vector3 -F -h report=svf
LDFLAGS		=
HEADERS		= s8s.h

CSPARSE_OBJS 	= m8sc.o s8sc.o randc.o p8sc.o c8sc.o r8sc.o utilc.o

all :  		bench8sc

bench8sc : 	$(CSPARSE_OBJS) Makefile s8s.h
		$(CC) $(LDFLAGS)  -o bench8sc $(CSPARSE_OBJS) -lm

m8sc.o : 	m8sc.c $(HEADERS) 
		$(CC) $(CFLAGS) -c m8sc.c

s8sc.o : 	s8sc.c 
		 $(CC) $(CFLAGS) -c s8sc.c

p8sc.o:	 	p8sc.c 
		 $(CC) $(CFLAGS) -c p8sc.c

c8sc.o:	 	c8sc.c 
		 $(CC) $(CFLAGS) -c c8sc.c

r8sc.o:	 	r8sc.c 
		 $(CC) $(CFLAGS) -c r8sc.c

utilc.o: 	utilc.c 
		 $(CC) $(CFLAGS) -c utilc.c

randc.o: 	randc.c 
		 $(CC) $(CFLAGS) -c randc.c

clean :
	rm -f *.o bench8sc bench8s
