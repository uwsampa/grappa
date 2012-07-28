# Benchmark XOR

# CVS info
# $Date: 2010/04/08 20:04:47 $
# $Revision: 1.17 $
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
XOR_DIR		= $(FARM_ROOT)/xor

#VPATH = $(CM_DIR) $(XOR_DIR)

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
CM_OBJS = $(CM_DIR)/for_drivers.o $(CM_DIR)/error_routines.o $(CM_DIR)/bm_timers.o $(CM_DIR)/brand.o $(CM_DIR)/uqid.o

XOR_OBJS = ./xor.o ./driver.o ./no_opt.o ./print_cmplr_flags.o

OBJS = $(XOR_OBJS) $(CM_OBJS)
# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS  =

CM_INC   = $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bench.h $(CM_DIR)/bm_timers.h \
             $(CM_DIR)/error_routines.h  $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:			xor

xor:			$(XOR_OBJS) $(XOR_DIR)/xor.c ./cflags.h $(CM_INC)
			$(CC) $(CFLAGS) -o xor $(OBJS)  $(LDFLAGS)

driver.o:		$(XOR_DIR)/driver.c $(XOR_DIR)/makefile $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(XOR_DIR) -c $<

xor.o:			$(XOR_DIR)/xor.c $(XOR_DIR)/makefile $(CM_INC)
			success=0 && $(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(XOR_DIR) -c $< && success=1 ;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error; \
				exit -1; \
			fi

print_cmplr_flags.o:	$(CM_DIR)/print_cmplr_flags.c $(XOR_DIR)/makefile cflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -I$(CM_DIR) -I$(XOR_DIR) -c $<


no_opt.o: 		$(XOR_DIR)/no_opt.c
			$(CC) $(CFLAGS) -I$(CM_DIR) -I$(XOR_DIR) -c $<

# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation.


cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > ./cflags.h


# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(XOR_DIR) ./driver.c ./xor.c


# Check the output against a known case
# Runs on an MPP,Farm, or Workstation
# The format for output is free form text or space separated columns
check:		./xor
		if \
                     test "$(LAUNCHER)" == "ressub"; \
                then \
                    echo "./xor 1401 81 33554432 " > resin ; \
                    /bin/cp ./xor $(RES_WK_DIR) ; \
                    ressub -label xor $(RES_POOL) $(PLT_LST) -f resin -o xor_1401_check_output -e xor_err \
                        -work_dir $(RES_WK_DIR) > ressub_out ; \
                    /bin/rm -f $(RES_WK_DIR)/xor ; \
                else \
                    $(LAUNCHER) ./xor 1401 81 33554432 > xor_1401_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 >  1) print $$1 , "checked failed" ; \
                          }' ./xor_1401_check_output > tiny_xor_1401_diffs ; \
                else \
                     /usr/bin/diff ./xor_1401_correct_output ./xor_1401_check_output > xor_1401_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./xor_1401_diffs > tiny_xor_1401_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_xor_1401_diffs\n"

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./xor $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(XOR_OBJS) ./*~ ./#*# ./cflags.h ./xor_1401_check_output \
                    ./xor_1401_check_output ./xor_1401_diffs ./tiny_xor_1401_diffs ./*.oo \
                    *.o ./resin ./xor_err ./ressub_out

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./xor ./makefile


TAGS:   ./xor.c ./driver.c ./no_opt.c
		etags -l c ./xor.c ./driver.c ./no_opt.c  --include=../support/TAGS



