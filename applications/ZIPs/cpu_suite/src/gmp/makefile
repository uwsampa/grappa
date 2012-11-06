# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 
#GMP_LIBS = /usr/local/lib
#GMP_INCLUDES = /usr/local/include


SUPPORT_DIR=../support
GMP_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
        $(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h

GMP_OBJS = ./driver.o ./print_cmplr_flags.o

OBJS = $(GMP_OBJS) $(SUPPORT_OBJS)

all:            gmp

clean:
		/bin/rm -f ../../bin/gmp $(GMP_OBJS) ./*~ ./#*# ./cflags.h

gmp:            $(GMP_OBJS) ./cflags.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) $(UID) -o ../../bin/gmp $(OBJS)  $(LDFLAGS)\
		      -L$(LIBS)  -lgmp

driver.o:       $(GMP_DIR)/driver.c $(GMP_DIR)/makefile $(SUPPORT_INC)
		success=0 && $(CC) $(CFLAGS) $(UID)  -I$(INCLUDES) -c $< && success=1;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi

print_cmplr_flags.o: $(SUPPORT_DIR)/print_cmplr_flags.c $(GMP_DIR)/makefile ./cflags.h
		$(CC) $(CFLAGS) -DHAS_CFLAGS  -c $<

# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation

cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h
