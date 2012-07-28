# CVS info  
# $Date: 2005/07/14 20:44:59 $    
# $Revision: 1.1 $
# $RCSfile: Makefile,v $ 
# $Name:  $  
#

#NPES = 19
NPES = 4
LTSIZE = 27

#CC = sgiupc
CC = upcc
#CFLAGS = -fshared -fthreads-$(NPES) -X $(NPES) -O2 -DLTABSIZE=$(LTSIZE)
#CFLAGS = -mabi=64 -mips4 -fupc-threads-$(NPES) -x upc -O2 -DLTABSIZE=$(LTSIZE)
#CFLAGS = -LANG:upc_threads=$(NPES) -O2 -DLTABSIZE=$(LTSIZE)
#LDFLAGS= -fupc-threads-$(NPES) -O2 -DLTABSIZE=$(LTSIZE)
CFLAGS= -T=$(NPES) -Wc,-O2 -DLTABSIZE=$(LTSIZE)
#CFLAGS= -T=$(NPES) -Wc,-O2 -DLTABSIZE=$(LTSIZE)
LDFLAGS= $(CFLAGS)
#OBJS = guppie.upc_gs.o upc_gs.o
SRC  = guppie.upc
OBJS = guppie.o
TARGET = guppie

$(TARGET): $(OBJS) makefile
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	cp guppie ../bin/guppie
$(OBJS): $(SRC) makefile
	$(CC) $(CFLAGS) $(SRC) -c -o $(OBJS)


clean:
	rm -f *.o guppie 


cleanall: clean
	rm -f ../bin/guppie

run:
	srun -n 2 ../bin/guppie
