# Benchmark SORT

# CVS info
# $Date: 2010/04/08 20:00:32 $
# $Revision: 1.21 $
# $RCSfile: makefile_template.mk,v $
# $State: Stab $

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

# BASECFLAGS = -I. -I../support

SHELL		= /bin/sh

FARM_ROOT	= ~/your path goes here or set FARM_ROOT in environment
CM_DIR		= $(FARM_ROOT)/support
A_SORT_DIR	= $(FARM_ROOT)/sort

INSTALL_DIR	= $(FARM_ROOT)/bin

#VPATH = $(CM_DIR) $(A_SORT_DIR)

RES_DIR		=
#RES_POOL	= -P xxxx
# Default pool
RES_POOL	=
RES_WK_DIR      =

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

A_SORT_OBJS = ./a_sort.o ./driver.o ./psort.o ./print_cmplr_flags.o

OBJS = $(A_SORT_OBJS) $(CM_OBJS)

SmallSortSz = 96
PSZ1  =     6
PSZ3  =     8

# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS  =

CM_INC   = $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bench.h $(CM_DIR)/bm_timers.h \
             $(CM_DIR)/error_routines.h  $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:		a_sort

a_sort:		$(A_SORT_OBJS) $(A_SORT_DIR)/a_sort.c ./cflags.h $(CM_INC)
		$(CC) $(CFLAGS) -DSmallSortSz=$(SmallSortSz) -DPSZ1=$(PSZ1) -DPSZ3=$(PSZ3) \
                    -o a_sort $(OBJS) -lm $(LDFLAGS)
		date

# Generate the a_sort binaries with a specific compiler (e.g icc) and put them
# in directories dedicated to that compiler


driver.o:  $(A_SORT_DIR)/driver.c $(A_SORT_DIR)/makefile $(CM_INC)
			$(CC) $(CFLAGS)  $(OFMT) $(UID) -I$(CM_DIR) -I$(A_SORT_DIR) -c $<

a_sort.o:	$(A_SORT_DIR)/a_sort.c $(A_SORT_DIR)/makefile $(CM_INC)
		success=0 && $(CC) $(CFLAGS)  $(OFMT) $(UID) -I$(CM_DIR) -I$(A_SORT_DIR) -c $< && success=1 ;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi

print_cmplr_flags.o: $(CM_DIR)/print_cmplr_flags.c $(A_SORT_DIR)/makefile ./cflags.h
		$(CC) $(CFLAGS) -DHAS_CFLAGS -I$(CM_DIR) -I$(A_SORT_DIR) -c $<


# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation


cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h


# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(A_SORT_DIR) ./driver.c ./a_sort.c

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./a_sort $(INSTALL_DIR)

# Check the output against a known case
# Runs on an MPP,Farm, or Workstation
# The format for output is free form text or space separated columns
check:		./a_sort
		if \
                     test "$(LAUNCHER)" == "ressub" ; \
                then \
                     /bin/cp ./a_sort $(RES_WK_DIR) ; \
                     echo "./a_sort  2101 11587  16 " > resin ; \
                     ressub -label sort $(RES_POOL) $(PLT_LST) -f resin -o a_sort_2101_check_output -e sort_err \
                         -work_dir $(RES_WK_DIR) > ressub_out ; \
                     /bin/rm -f $(RES_WK_DIR)/a_sort ; \
                else \
                     $(LAUNCHER) ./a_sort  2101 11587  16 > ./a_sort_2101_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 >  1) print $$1 , "checked failed" ; \
                          }' ./a_sort_2101_check_output > tiny_a_sort_2101_diffs ; \
                else \
                     /usr/bin/diff ./a_sort_2101_correct_output ./a_sort_2101_check_output > a_sort_2101_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---|second" ./a_sort_2101_diffs > tiny_a_sort_2101_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_a_sort_2101_diffs\n"

.PHONY: clean
clean:
		/bin/rm -f *.o $(OBJS) ./a_sort_2101_check_output ./a_sort_2101_diffs ./tiny_a_sort_2101_diffs ./*.oo \
                     ./resin ./sort_err ./ressub_out 

.PHONY: clobber
clobber:	clean
		/bin/rm -f a_sort makefile

TAGS:   ./a_sort.c ./driver.o ./psort.c
		etags -l c  ./a_sort.c ./psort.c ./driver.c --include=../support/TAGS

