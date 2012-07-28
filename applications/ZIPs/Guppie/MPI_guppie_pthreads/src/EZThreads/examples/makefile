# CVS info  
# $Date: 2005/06/07 18:57:43 $    
# $Revision: 1.1 $
# $RCSfile: Makefile.orig,v $ 
# $Name:  $    
#

INCDIR = ../include
LIBDIR = ../lib

# setup for digital unix
CC = cc
CFLAGS = -pthread -I$(INCDIR)
LDFLAGS = -L$(LIBDIR) -lEZThreads

OBJS = EZtest.o
TARGET = EZtest
$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS)

cleanall:
	rm -f $(OBJS) $(TARGET)
