CC=gcc

#CFLAGS = -g -fast -axSSE4.2 -I.

#ifeq "$(CC)" "gcc"
CFLAGS = -O3 -I. 
#-funroll-all-loops -finline-functions -fprefetch-loop-arrays -ftree-vectorize -ffast-math -flto -fwhole-program 
#endif 

CFLAGS += -DVEC_WIDTH=128

SRCS = mmbs2.c mmbs2_driver.c tilt.c
INCL = bench.h  mm_ops.h mmbs2.h
MMBS2_OBJS = mmbs2_driver.o mmbs2.o tilt.o
BINS = mmbs2 

all: $(BINS)

mmbs2:		$(MMBS2_OBJS)
		$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(MMBS2_OBJS)

mmbs2_driver.o:	mmbs2_driver.c mmbs2.h mm_ops.h
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

mmbs2.o:	mmbs2.c mmbs2.h mm_ops.h
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

$(OBJS):	makefile

clean:		
		/bin/rm -f $(MMBS2_OBJS) $(BINS)
