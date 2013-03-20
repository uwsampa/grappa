CC = cc
LD = $(CC)
CFLAGS  = -O3 -pl ssca2.pl -par
LDFLAGS = -pl ssca2.pl 

OBJS =	main.o globals.o getUserParameters.o genScalData.o \
	computeGraph.o getStartLists.o findSubGraphs.o centrality.o \
	timer.o

LIBS = -lm -lprand

all: $(OBJS)
	$(LD) $(LDFLAGS) -o ssca2 $(OBJS) $(LIBS)

globals.o:		globals.c globals.h
	$(CC) $(CFLAGS) -c globals.c

getUserParameters.o:	getUserParameters.c globals.h
	$(CC) $(CFLAGS) -c getUserParameters.c

genScalData.o:	genScalData.c globals.h defs.h
	$(CC) $(CFLAGS) -c genScalData.c

computeGraph.o:	computeGraph.c globals.h defs.h
	$(CC) $(CFLAGS) -c computeGraph.c

getStartLists.o:	getStartLists.c globals.h defs.h
	$(CC) $(CFLAGS) -c getStartLists.c

findSubGraphs.o:	findSubGraphs.c globals.h defs.h
	$(CC) $(CFLAGS) -c findSubGraphs.c

centrality.o:	centrality.c globals.h defs.h
	$(CC) $(CFLAGS) -c centrality.c

timer.o:	timer.c
	$(CC) $(CFLAGS) -c timer.c

main.o:		main.c globals.h defs.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o ssca2.pl ssca2
