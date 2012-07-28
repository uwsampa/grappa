# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 

SUPPORT_DIR=../support
XOR_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h

XOR_OBJS = ./xor.o ./driver.o ./no_opt.o ./print_cmplr_flags.o

OBJS = $(XOR_OBJS) $(SUPPORT_OBJS)

all:		xor

clean:
		/bin/rm -f ../../bin/xor $(XOR_OBJS) ./*~ ./#*# ./cflags.h


xor:                    $(XOR_OBJS) $(XOR_DIR)/xor.c ./cflags.h $(SUPPORT_INC)
			$(CC) $(CFLAGS) -o ../../bin/xor $(OBJS) $(LDFLAGS)

driver.o:               $(XOR_DIR)/driver.c $(XOR_DIR)/makefile $(SUPPORT_INC)
			$(CC) $(CFLAGS) $(UID) -c $<

no_opt.o:               $(XOR_DIR)/no_opt.c 
			$(CC) $(CFLAGS) $(UID) -c $<

xor.o:                  $(XOR_DIR)/xor.c $(XOR_DIR)/makefile $(SUPPORT_INC)
			success=0 && $(CC) $(CFLAGS) $(UID) -c $< && success=1;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error; \
				exit -1; \
			fi

print_cmplr_flags.o:    $(SUPPORT_DIR)/print_cmplr_flags.c $(XOR_DIR)/makefile ./cflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -c $<


cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

