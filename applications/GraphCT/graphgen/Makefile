include ../Make.inc

OBJS =	computeGraph.o genScalData.o gen_RMAT_graph.o getUserParameters.o \
		globals.o graph-manip.o timer.o xmalloc.o xmt-luc-blech.o undirected.o

all:	gen_RMAT_graph

gen_RMAT_graph:	$(OBJS) $(IOLIB) $(COMPATLIB)
	$(LD) $(LDFLAGS) -o gen_RMAT_graph $(OBJS) $(COMPATLIB) $(LIBS)

computeGraph.o:		computeGraph.c defs.h
genScalData.o:		genScalData.c defs.h globals.h
gen_RMAT_graph.o:		gen_RMAT_graph.c defs.h globals.h
getUserParameters.o:	getUserParameters.c globals.h
globals.o:		globals.c globals.h
graph-manip.o:	graph-manip.c defs.h
timer.o:		timer.c globals.h
xmalloc.o:		xmalloc.c
undirected.o:	undirected.c

xmt-luc-blech.o:    xmt-luc-blech.cc
	$(CXX) $(CFLAGS) -c -o $@ $<

$(IOLIB) $(COMPATLIB):
	$(MAKE) -C $(IOLIB_DIR)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o GraphCT.pl gen_RMAT_graph
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi
