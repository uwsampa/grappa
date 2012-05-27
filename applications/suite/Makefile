include Make.inc

EXE = suite.exe

OBJS = 	computeGraph.o \
		connectedComponents.o \
		genScalData.o \
		globals.o \
		graph-manip.o \
		pathIsomorphism.o \
		timer.o \
		triangles.o \
		util.o \
		xmalloc.o \
		centrality.o

all: $(EXE)
	
$(EXE): main.o $(OBJS) $(COMPATLIB) $(IOLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

computeGraph.o: computeGraph.c defs.h
connectedComponents.o: connectedComponents.c defs.h
genScalData.o: genScalData.c defs.h
globals.o:		globals.c defs.h
graph-manip.o: graph-manip.c defs.h
pathIsomorphism.o: pathIsomorphism.c defs.h
timer.o: timer.c defs.h
triangles.o: triangles.c defs.h
util.o: util.c defs.h
xmalloc.o: xmalloc.c defs.h
centrality.o: centrality.c defs.h


$(IOLIB) $(COMPATLIB):
	$(MAKE) -C $(IOLIB_DIR)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(EXE){,.pl,.ap2} profile.out trace.out
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi

distclean:
	rm -f *.o $(EXE)
	-if test -n "$(IOLIB_DIR)" ; then make -C $(IOLIB_DIR) clean; fi

