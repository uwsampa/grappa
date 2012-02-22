include Make.inc

OBJS =	globals.o getUserParameters.o genScalData.o \
	computeGraph.o getStartLists.o findSubGraphs.o centrality.o \
	timer.o modularity.o distribution.o conductance.o \
	connectedComponents.o kcentrality.o graphio.o kcore.o \
	clustering_local.o clustering_global.o clustering_trans.o \
	parse_DIMACS_graph.o \
	cmdline.o xmalloc.o graph-manip.o xmt-luc-blech.o \
	script_reader.o undirected.o bfs.o diameter.o partitioning.o \
	community_modularity.o community_clustering.o community_kullback_liebler.o \
	community_conductance.o read-graph-el.o

all:	GraphCT GraphCT-CLI GraphCT-script

GraphCT:	main.o $(OBJS) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o GraphCT main.o $(OBJS) $(COMPATLIB) $(IOLIB) $(LIBS)

GraphCT-CLI:	main_single.o $(OBJS) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o GraphCT-CLI main_single.o $(OBJS) $(COMPATLIB) $(IOLIB) $(LIBS)

GraphCT-script:	main_script.o $(OBJS) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o GraphCT-script main_script.o $(OBJS) $(COMPATLIB) $(IOLIB) $(LIBS)

globals.o:		globals.c globals.h
getUserParameters.o:	getUserParameters.c globals.h
genScalData.o:	genScalData.c globals.h defs.h
computeGraph.o:	computeGraph.c globals.h defs.h
getStartLists.o:	getStartLists.c globals.h defs.h
findSubGraphs.o:	findSubGraphs.c globals.h defs.h
centrality.o:	centrality.c globals.h defs.h
timer.o:	timer.c
modularity.o:	modularity.c globals.h defs.h
distribution.o:	distribution.c globals.h defs.h computeGraph.c
conductance.o:	conductance.c globals.h defs.h
connectedComponents.o:	connectedComponents.c globals.h defs.h
kcentrality.o:	kcentrality.c globals.h defs.h
graphio.o:	graphio.c globals.h defs.h read-graph-el.h
clustering_global.o:	clustering_global.c globals.h defs.h distribution.c
clustering_local.o:	clustering_local.c globals.h defs.h clustering_global.c
clustering_trans.o:	clustering_trans.c globals.h defs.h clustering_global.c
kcore.o: kcore.c defs.h globals.h
main.o:		main.c globals.h defs.h
main_single.o:	main_single.c globals.h defs.h
parse_DIMACS_graph.o:	parse_DIMACS_graph.c defs.h computeGraph.c
cmdline.o:	cmdline.c defs.h
xmalloc.o:		xmalloc.c defs.h
graph-manip.o:	graph-manip.c
script_reader.o:	script_reader.c
undirected.o:	undirected.c defs.h globals.h
bfs.o: 	bfs.c defs.h globals.h
diameter.o:	diameter.c defs.h globals.h
partitioning.o:	partitioning.c defs.h globals.h
community_modularity.o:	community_modularity.c defs.h globals.h
community_clustering.o: community_clustering.c defs.h globals.h
community_kullback_liebler.o:	community_kullback_liebler.c defs.h globals.h
community_conductance.o: 	community_conductance.c defs.h globals.h
read-graph-el.o:	read-graph-el.c read-graph-el.h

xmt-luc-blech.o:    xmt-luc-blech.cc
	$(CXX) $(CFLAGS) -c -o $@ $<

$(IOLIB) $(COMPATLIB):
	$(MAKE) -C $(IOLIB_DIR)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o GraphCT.pl GraphCT GraphCT-CLI GraphCT-script
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi

distclean:
	rm -f *.o GraphCT.pl GraphCT GraphCT-CLI GraphCT-script
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi
	-if test -n "$(PARSER_DIR)" ; then make -C $(PARSER_DIR) clean; fi
	-if test -n "$(GRAPHGEN_DIR)" ; then make -C $(GRAPHGEN_DIR) clean; fi

