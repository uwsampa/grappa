include ../Make.inc

OBJS =	parse_DIMACS_graph.o xmalloc.o xmt-luc-blech.o

all:	parse_DIMACS_graph

parse_DIMACS_graph:	$(OBJS) $(IOLIB) $(COMPATLIB)
	$(LD) $(LDFLAGS) -o parse_DIMACS_graph $(OBJS) $(COMPATLIB) $(IOLIB) $(LIBS)

parse_DIMACS_graph.o:		parse_DIMACS_graph.c defs.h xmt-luc.h compat/xmt-ops.h

xmalloc.o:		xmalloc.c defs.h

xmt-luc-blech.o:    xmt-luc-blech.cc
	$(CXX) $(CFLAGS) -c -o $@ $<

$(IOLIB) $(COMPATLIB):
	$(MAKE) -C $(IOLIB_DIR)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o GraphCT.pl parse_DIMACS_graph
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi
