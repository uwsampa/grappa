# Benchmark BS2

# CVS info
# $Date: 2010/04/08 19:53:17 $
# $Revision: 1.22 $
# $RCSfile: makefile_template.mk,v $
# $State: Stable

#
#    The default 'make` action is to compile the benchmark programs in their
#  individual directories and leave the executable and '.o' files there.
#  Subroutines that are used by several benchmarks are in the directory
#  'support'. Again, the default is to leave the '.o' file in the source
#  directory.
#    Several make macros are provided to facilitate customization.  Specifically,
#  BIN_ROOT can be made different than SRC_ROOT so binaries are stored apart from
#  the source. This may be useful when building the benchmarks with different
#  compilers.

# compiler     cc                                  icc                          pathc

#          farm_cpusuite                       farm_cpusuite               farm_cpusuite
#     +-------+----+--...--+------+           +-------+---...--+          +-------+----...--+
#  support   cba  xor ... mapin  cc_bin    support  cba  ... icc_bin   support  cba  ... pathc_bin
#
#
#
#

# BASECFLAGS = -I. -I../support

SHELL		= /bin/sh

FARM_ROOT	= ~/your path goes here or set FARM_ROOT in environment
CM_DIR		= $(FARM_ROOT)/support
BS2_DIR		= $(FARM_ROOT)/bs2

#VPATH = $(CM_DIR) $(BS2_DIR)

RES_DIR		=
#RES_POOL	= -P xxxx
RES_POOL	=
RES_WK_DIR      =

# set PLT_LST in environment
#PLT_LST		= -platform_list yyy   reshosts -sum => x86_64-cpu-linux6
#PLT_LST		= -platform_list x86_64-cpu-linux6

#Shortened output conditional compile
OFMT            =
#Unique identifier conditional compile
UID             =

CC		=
CFLAGS		=

# NOTE: If using modules, like on a Cray, should be set by hand
BIN_PRFX        = $(CC)
INSTALL_DIR	= $(FARM_ROOT)/$(CC)_bin

SPLINT		= /usr/bin/splint
SPFLAGS		= -weak -posix-lib

# Common routines used by all benchmarks. Error routines, timers, and random numbers
CM_OBJS		= $(CM_DIR)/for_drivers.o $(CM_DIR)/error_routines.o $(CM_DIR)/bm_timers.o \
                  $(CM_DIR)/brand.o $(CM_DIR)/uqid.o

BS2_OBJS	= ./driver.o ./bs2.o ./print_cmplr_flags.o ./tilt.o

OBJS		= $(BS2_OBJS) $(CM_OBJS)

F90		=
FFLAGS		=

# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS  =

CM_INC   = $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bench.h $(CM_DIR)/bm_timers.h \
             $(CM_DIR)/error_routines.h  $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:			bs2

# C code tilt
bs2:			$(BS2_OBJS) $(BS2_DIR)/bs2.f ./cflags.h ./fflags.h $(CM_INC)
			$(CC) $(CFLAGS) -o bs2 $(OBJS)  $(LDFLAGS) $(LDFLAGS)
			date
# C code
driver.o:		$(BS2_DIR)/driver.c $(BS2_DIR)/makefile $(CM_INC)
			success=0 && $(CC) $(CFLAGS)  $(OFMT) $(UID) -I$(CM_DIR) -I$(BS2_DIR) -c $< && success=1; \
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

tilt.o:			$(BS2_DIR)/tilt.c $(BS2_DIR)/makefile  $(CM_INC)
			$(CC) $(CFLAGS)  $(OFMT) $(UID) -I$(CM_DIR) -I$(BS2_DIR) -c $<



# To find the symbol name of `bs2` that the C driver program will need
# run `make bs2.s` and find the symbol in the assembly
bs2.s:
			$(F90) -c -S ./bs2.f -o symbol_name
# Fortran code
bs2.o:			$(BS2_DIR)/bs2.f $(BS2_DIR)/makefile
			$(F90) $(FFLAGS) -I$(CM_DIR) -I$(BS2_DIR) -c $<

print_cmplr_flags.o:	$(CM_DIR)/print_cmplr_flags.c $(BS2_DIR)/makefile cflags.h fflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -DHAS_FFLAGS -I$(CM_DIR) -I$(BS2_DIR) -c $<

# Save C compiler and flags in include file cflags.h
cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Save Fortran compiler and flags in include file fflags.h
fflags.h:
		echo "char fflags[] = " \"$(F90) $(FFLAGS) \" \; > fflags.h

# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(BS2_DIR) ./driver.c ./bs2.c

check:		./bs2
		if \
                     test "$(LAUNCHER)" == "ressub" ; \
                then \
                     echo "./bs2   301  2575027 "  >  bs2_res_in ; \
                     /bin/cp ./bs2 $(RES_WK_DIR) ; \
                     ressub -label bs2 $(RES_POOL) $(PLT_LST) -f bs2_res_in -o bs2_301_check_output \
                        -e bs2_err -work_dir $(RES_WK_DIR) > ressub_out; \
                     /bin/rm -f $(RES_WK_DIR)/bs2 ; \
                else \
                     $(LAUNCHER) ./bs2   301  2575027 > bs2_301_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 == 2) print $$1 , "checked failed" ; \
                          }' ./bs2_301_check_output > tiny_bs2_301_diffs ; \
                else \
                     /usr/bin/diff ./bs2_301_correct_output ./bs2_301_check_output > bs2_301_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./bs2_301_diffs > tiny_bs2_301_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_bs2_301_diffs\n"

# Run code under RES
# sec_frm_epoch  returns the number of seconds since the epoch
# option -l is when the RES job is launched
# option -s is the number of second to sleep before timed code is started
check_with_RES:	./bs2
		echo "./bs2   301  2575027  -l `$(CM_DIR)/sec_frm_epoch`  -s 60 8 " > bs2_res_in
		$(RES_DIR)/ressub $(RES_POOL) $(PLT_LST) -f bs2_res_in \
                    -o bs2_res_stdout -e bs2_res_err > bs2_res_out

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./bs2 $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(BS2_OBJS) ./*~ ./#*# ./cflags.h ./fflags.h ./bs2.s ./bs2_301_diffs \
                    ./tiny_bs2_301_diffs ./bs2_res_in ./bs2_res_stdout ./bs2_res_err ./bs2_res_out \
                    ./bs2_301_check_output ./bs2_err ./ressub_out ./symbol_name

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./bs2 ./makefile

#Create a file of tag for the editor
TAGS:   ./bs2.f ./driver.c ./tilt.c
		etags --declarations -l c ./driver.c ./tilt.c -l fortran ./bs2.f --include=../support/TAGS


