include Makefile.var

TARGET = SSCA2

OBJS = SSCA2.o init.o utils.o genScalData.o gen2DTorus.o \
    computeGraph.o getStartLists.o findSubGraphs.o  \
    betweennessCentrality.o

.c.o: defs.h Makefile
	$(CC) $(INC) $(CFLAGS) -c $<


all: sprng $(OBJS) defs.h Makefile
	$(CC) $(INC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIB)

sprng: 
	(cd sprng2.0; $(MAKE); cd ..)

cleansprng: 
	(cd sprng2.0; $(MAKE) clean; cd ..)

clean: cleansprng
	rm -f *.o *~ $(TARGET) core*
