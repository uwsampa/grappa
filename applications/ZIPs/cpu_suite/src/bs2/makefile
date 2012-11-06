# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 
F90 = ifort
FFLAGS = -O3 -r8 -i8 -nofor-main


SUPPORT_DIR=../support
BS2_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
        $(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
        $(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
        $(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
        $(SUPPORT_DIR)/iobmversion.h

BS2_OBJS        = ./driver.o ./bs2.o ./print_cmplr_flags.o ./tilt.o

OBJS            = $(BS2_OBJS) $(SUPPORT_OBJS)

all:		bs2

clean:
		/bin/rm -f ../../bin/bs2 $(BS2_OBJS) ./*~ ./#*# ./cflags.h

bs2:                    $(BS2_OBJS) $(BS2_DIR)/bs2.f ./cflags.h ./fflags.h $(SUPPORT_INC)
			$(CC) $(CFLAGS) -o ../../bin/bs2 $(OBJS)  $(LDFLAGS)


driver.o:               $(BS2_DIR)/driver.c $(BS2_DIR)/makefile $(SUPPORT_INC)
			success=0 && $(CC) $(CFLAGS)  $(UID) -c $< && success=1; \
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

tilt.o:                 $(BS2_DIR)/tilt.c $(BS2_DIR)/makefile  $(SUPPORT_INC)
			$(CC) $(CFLAGS) $(UID) -c $<



# To find the symbol name of `bs2` that the C driver program will need
# run `make bs2.s` and find the symbol in the assembly
bs2.s:
			$(F90) -c -S ./bs2.f -o symbol_name
# Fortran code
bs2.o:                  $(BS2_DIR)/bs2.f $(BS2_DIR)/makefile
			$(F90) $(FFLAGS) -I$(SUPPORT_DIR) -I$(BS2_DIR) -c $<
print_cmplr_flags.o:    $(SUPPORT_DIR)/print_cmplr_flags.c $(BS2_DIR)/makefile cflags.h fflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -DHAS_FFLAGS -c $<

# Save C compiler and flags in include file cflags.h
cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Save Fortran compiler and flags in include file fflags.h
fflags.h:
		echo "char fflags[] = " \"$(F90) $(FFLAGS) \" \; > fflags.h

