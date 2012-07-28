# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 


SUPPORT_DIR=../support
CBA_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h

CBA_OBJS = ./cba.o ./driver.o ./print_cmplr_flags.o

OBJS = $(CBA_OBJS) $(SUPPORT_OBJS)

all:            cba

clean: 		
		/bin/rm -f cba $(CBA_OBJS) ./*~ ./#*# ./cflags.h

cba:            $(CBA_OBJS) $(CBA_DIR)/cba.c ./cflags.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) -o cba $(OBJS)  $(LDFLAGS)


driver.o:  $(CBA_DIR)/driver.c $(CBA_DIR)/Makefile $(CBA_DIR)/cba.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) $(UID) -c $<

cba.o:  $(CBA_DIR)/cba.c $(CBA_DIR)/Makefile $(CBA_DIR)/cba.h $(SUPPORT_INC)
		success=0 && $(CC) $(CFLAGS) $(UID) -c $< && success=1;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi

print_cmplr_flags.o: $(SUPPORT_DIR)/print_cmplr_flags.c $(CBA_DIR)/Makefile cflags.h
		$(CC) $(CFLAGS) -DHAS_CFLAGS -c $<

# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation

cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

