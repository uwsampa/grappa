# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 


SUPPORT_DIR=../support
GTSC_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h

GT_LOCAL_OBJS = ./gt.o ./driver_gt.o ./print_cmplr_flags.o
SC_LOCAL_OBJS = ./sc.o ./driver_sc.o ./print_cmplr_flags.o

GT_OBJS = $(GT_LOCAL_OBJS) $(SUPPORT_OBJS)
SC_OBJS = $(SC_LOCAL_OBJS) $(SUPPORT_OBJS)

all: 	gt sc

clean:
			/bin/rm -f ../../bin/gt ../../bin/sc $(GT_LOCAL_OBJS) $(SC_LOCAL_OBJS) ./*~ ./#*# ./cflags.h

gt: 			$(GT_LOCAL_OBJS) $(GTSC_DIR)/gt.c ./cflags.h $(SUPPORT_INC)
			$(CC) $(CFLAGS) -o ../../bin/gt $(GT_OBJS)  $(LDFLAGS)

driver_gt.o:            $(GTSC_DIR)/driver_gt.c $(GTSC_DIR)/makefile $(SUPPORT_INC)
			$(CC) $(CFLAGS) $(UID) -c $<

gt.o:			$(GTSC_DIR)/gt.c $(GTSC_DIR)/makefile $(SUPPORT_INC)
			$(CC) $(CFLAGS) $(UID) -c $<

print_cmplr_flags.o:    $(SUPPORT_DIR)/print_cmplr_flags.c $(GTSC_DIR)/makefile ./cflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -c $<

# Scatter

sc:             $(SC_LOCAL_OBJS) $(GTSC_DIR)/sc.c ./cflags.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) -o ../../bin/sc $(SC_OBJS) $(LDFLAGS)

driver_sc.o:    $(GTSC_DIR)/driver_sc.c $(GTSC_DIR)/makefile $(SUPPORT_INC)
		$(CC) $(CFLAGS) $(UID) -c $<

sc.o:           $(GTSC_DIR)/sc.c $(GTSC_DIR)/makefile $(SUPPORT_INC)
		success=0 && $(CC) $(CFLAGS) $(UID) -c $< && success=1;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi

# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation

cflags.h:
	echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h
