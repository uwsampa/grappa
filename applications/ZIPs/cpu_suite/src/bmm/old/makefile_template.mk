# Benchmark BMM

# CVS info
# $Date: 2010/04/08 19:51:52 $
# $Revision: 1.27 $
# $RCSfile: makefile_template.mk,v $
# $State: Stab $

#
#    The default 'maker' action is to compile the benchmark programs in their
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

# BASECFLAGS = -I. -I../support
# could add big page flags if applicable above

SHELL		= /bin/sh

FARM_ROOT	= ~/your path goes here or set FARM_ROOT in environment
CM_DIR		= $(FARM_ROOT)/support
BMM_DIR		= $(FARM_ROOT)/bmm

#VPATH = $(CM_DIR) $(BMM_DIR)


RES_DIR		=
#RES_POOL	= -P xxxx
# Default pool
RES_POOL	=
RES_WK_DIR      =
# Force to wait a given time
RES_SYNCH       =
# For computing synchronization time
SYNCHWAIT       =

# set PLT_LST in environment
#PLT_LST		= -platform_list yyy   reshosts -sum => x86_64-cpu-linux6
#PLT_LST		= -platform_list x86_64-cpu-linux6

#Shortened output conditional compile
OFMT            =
#Unique identifier conditional compile
UID             =

CC	=
CFLAGS	=

# NOTE: If using modules, like on a Cray, should be set by hand
BIN_PRFX        = $(CC)
INSTALL_DIR	= $(FARM_ROOT)/$(CC)_bin

SPLINT	= /usr/bin/splint
SPFLAGS = -weak -posix-lib

# Common routines used by all benchmarks. Error routines, timers, and random numbers
CM_OBJS = $(CM_DIR)/for_drivers.o $(CM_DIR)/error_routines.o $(CM_DIR)/bm_timers.o \
          $(CM_DIR)/brand.o $(CM_DIR)/uqid.o

BMM_OBJS = bmm.o driver.o print_cmplr_flags.o

OBJS = $(BMM_OBJS) $(CM_OBJS)

# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS =

CM_INC   = $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bench.h $(CM_DIR)/bm_timers.h \
             $(CM_DIR)/error_routines.h $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:		bmm

bmm:		$(BMM_OBJS) $(BMM_DIR)/bmm.c ./cflags.h $(CM_INC)
		$(CC) $(CFLAGS) -o bmm $(OBJS) $(LDFLAGS)
		date

# Generate the bmm binaries with a specific compiler (e.g icc) and put them
# in directories dedicated to that compiler

driver.o:  		$(BMM_DIR)/driver.c $(BMM_DIR)/makefile $(BMM_DIR)/bmm.h $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(BMM_DIR) -c $<

bmm.o:			$(BMM_DIR)/bmm.c $(BMM_DIR)/makefile $(BMM_DIR)/bmm.h $(BMM_DIR)/bmm_cksm_times.h $(CM_INC)
			success=0 && $(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(BMM_DIR) -c $< && success=1; \
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

print_cmplr_flags.o: 	$(CM_DIR)/print_cmplr_flags.c $(BMM_DIR)/makefile ./cflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -I$(CM_DIR) -I$(BMM_DIR) -c $<


# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation


cflags.h:
			echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h



# Check unannotated C code (can't handle ~)
splint:
			$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(BMM_DIR) ./driver.c ./bmm.c


# Check the output against a known case
# Runs on an MPP,Farm, or Workstation
# The format for output is free form text or space separated columns
check:		./bmm
		if \
                     test "$(LAUNCHER)" == "ressub" ; \
                then \
                     if \
                        test "$(RES_SYNCH)" == "bytime" ; \
                     then \
                        echo "./bmm   1   55179 -l $(shell date +%s) -s $(SYNCHWAIT)" > resin ; \
                     else \
                        echo "./bmm   1   55179 " > resin ; \
                     fi ; \
                     /bin/cp ./bmm $(RES_WK_DIR) ; \
                     ressub -label bmm $(RES_POOL) $(PLT_LST) -f resin -o bmm_1_check_output -e bmm_err -work_dir $(RES_WK_DIR) > ressub_out ; \
                     /bin/rm -f $(RES_WK_DIR)/bmm ; \
                else \
                     $(LAUNCHER) ./bmm   1   55179 > bmm_1_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 == 2) print $$1 , "checked failed" ; \
                            if ( $$3 == 3) print $$1 , "ran too fast" ; \
                            if ( $$3 == 4) print $$1 , "ran too slow" ; \
                          }' ./bmm_1_check_output > tiny_bmm_1_diffs ; \
                else \
                     /usr/bin/diff ./bmm_1_correct_output ./bmm_1_check_output > bmm_1_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./bmm_1_diffs > tiny_bmm_1_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_bmm_1_diffs\n"

# Check correctness of all benchmark runs
check_all:	./bmm
		./bmm   1   55179 > bmm_check_all_output

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./bmm $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(BMM_OBJS) ./*~ ./#*# ./cflags.h ./bmm_1_check_output ./bmm_1_diffs \
                     ./tiny_bmm_1_diffs ./resin ./bmm_err ./resin ./*.oo ./ressub_out

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./bmm ./makefile


TAGS:   ./bmm.c ./bmm.h ./driver.c
		etags -l c --declarations ./bmm.c ./bmm.h ./driver.c --include=../support/TAGS
