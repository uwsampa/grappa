CC = cc
LD = $(CC)
CFLAGS  = -par

SRCS =	main.c globals.c getUserParameters.c genScalData.c \
	computeGraph.c getStartLists.c findSubGraphs.c centrality.c \
	timer.c
SRCS2 =	utils.c graph_generator.c splittable_mrg.c graph-manip.c xmalloc.c

LIBS = -lm -lprand

ssca2:	$(SRCS) Makefile
	$(CC) -pl ssca2.pl $(CFLAGS) -DSSCA2 $(SRCS) $(LIBS) -o $@ 

kron:	$(SRCS) $(SRCS2) Makefile
	$(CC) -pl kron.pl $(CFLAGS) $(SRCS) $(SRCS2) $(LIBS) -o $@ 

clean:
	rm -f *.o *.pl ssca2 kron
