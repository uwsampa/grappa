# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 
F90 = ifort
FFLAGS = -O3 -r8 -i8 -nofor-main

SUPPORT_DIR=../support
TILT_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h

TILT_C_OBJS     = ./tilt.o ./driver.o ./print_cmplr_flags.o
TILT_F_OBJS     = ./ftilt.o ./fdriver.o ./print_cmplr_flags.o

OBJS_C          = $(TILT_C_OBJS) $(SUPPORT_OBJS)
OBJS_F          = $(TILT_F_OBJS) $(SUPPORT_OBJS)

all:		tilt ftilt

clean:		
		/bin/rm -f ../../bin/tilt ../../bin/ftilt $(TILT_C_OBJS) $(TILT_F_OBJS) ./*~ ./#*# ./cflags.h ./fflags.h


tilt:         	$(TILT_C_OBJS) $(TILT_DIR)/tilt.c ./cflags.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) -o ../../bin/tilt $(OBJS_C) $(LDFLAGS)

driver.o:       $(TILT_DIR)/driver.c $(TILT_DIR)/makefile $(SUPPORT_INC)
		$(CC) $(CFLAGS) $(UID) -DCSEED -c $<

tilt.o:         $(TILT_DIR)/tilt.c $(TILT_DIR)/makefile $(SUPPORT_INC)
		success=0 && $(CC) $(CFLAGS) $(UID) -c $< && success=1;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error; \
			exit -1; \
		fi

ftilt:          $(TILT_F_OBJS) $(TILT_DIR)/ftilt.f90 ./cflags.h ./fflags.h $(SUPPORT_INC)
		success=0 && $(F90) $(FFLAGS) -o ../../bin/ftilt $(OBJS_F) $(LDFLAGS) && success=1 ;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error; \
			exit -1; \
		fi

# change -DTILT="" below if needed for proper linking
fdriver.o:      $(TILT_DIR)/driver.c $(TILT_DIR)/makefile $(SUPPORT_INC)
		$(CC) $(CFLAGS) $(UID) -DFSEED -DHAS_CFLAGS -DHAS_FFLAGS \
			-DTILT="tilt_" -c ./driver.c -o fdriver.o

ftilt.o:        $(TILT_DIR)/ftilt.f90 $(TILT_DIR)/makefile $(SUPPORT_INC)
		$(F90) $(FFLAGS) -c $<


print_cmplr_flags.o:    $(SUPPORT_DIR)/print_cmplr_flags.c $(TILT_DIR)/makefile ./cflags.h ./fflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -DHAS_FFLAGS -c $<

# Save C compiler and flags in include file cflags.h
cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Save Fortran compiler and flags in include file fflags.h
fflags.h:
		echo "char fflags[] = " \"$(F90) $(FFLAGS) \" \; > fflags.h

# To find the symbol name of `ftilt` that the C driver program will need
# run `make ftilt.s` and find the symbol in the assembly
ftilt.s:
		$(F90) -c -S ./ftilt.f90 -o symbol_name
