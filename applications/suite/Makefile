include Make.inc

EXE = suite.exe
OBJS += 	computeGraph.o \
		connectedComponentsKahan.o \
		samplesort.o \
		genScalData.o \
		globals.o \
		graph-manip.o \
		pathIsomorphism.o \
		timer.o \
		triangles.o \
		util.o \
		xmalloc.o \
		centrality.o \
		compat/mersenne.o
OBJS2 = connectedComponentsKahan.o samplesort.o # write_graph.o
OBJS3 = connectedComponentsKahan.o  # write_graph.o

all: $(EXE) make_snaps.exe

$(EXE): main.o $(OBJS) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

# connectedComponentsKahan.o:
#	$(CXX) $(CFLAGS) -c $< -g

cctest: cctest.o $(OBJS2) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

# sandia_graph.o: sandia_graph.cc
#	$(CXX) $(CFLAGS) -c $< -g

sandia: sandia_graph.o $(OBJS3) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

make_snaps.exe: make_snaps.c
	gcc -std=c99 -o $@ $^

main.o: defs.h ../graph500/compatio.h
computeGraph.o: computeGraph.c defs.h
connectedComponentsKahan.o: connectedComponentsKahan.cc defs.h
genScalData.o: genScalData.c defs.h
globals.o:		globals.c defs.h
graph-manip.o: graph-manip.c defs.h
pathIsomorphism.o: pathIsomorphism.c defs.h
timer.o: timer.c defs.h
triangles.o: triangles.c defs.h
util.o: util.c defs.h
xmalloc.o: xmalloc.c defs.h
centrality.o: centrality.c defs.h compat/mersenne.c
compat/mersenne.o: compat/mersenne.c compat/mersenne.h

$(IOLIB) $(COMPATLIB):
	$(MAKE) -C $(IOLIB_DIR)

.cc.o:
	$(CXX) $(CFLAGS) -c $<
.c.o:
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f *.o $(EXE){,.pl,.ap2} profile.out trace.out cctest cctest.pl suite.pl
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi

distclean:
	rm -f *.o $(EXE) cctest
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi

