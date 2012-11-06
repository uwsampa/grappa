# Benchmark CBA

# CVS info
# $Date: 2010/04/08 19:54:36 $
# $Revision: 1.23 $
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
CBA_DIR		= $(FARM_ROOT)/cba

#VPATH = $(CM_DIR) $(CBA_DIR)

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

CBA_OBJS = ./cba.o ./driver.o ./print_cmplr_flags.o

OBJS = $(CBA_OBJS) $(CM_OBJS)

# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS  =

CM_INC   = $(CM_DIR)/bench.h $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bm_timers.h \
                $(CM_DIR)/error_routines.h $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:		cba

cba:		$(CBA_OBJS) $(CBA_DIR)/cba.c ./cflags.h $(CM_INC)
		$(CC) $(CFLAGS) -o cba $(OBJS)  $(LDFLAGS)
		date

# Generate the cba binaries with a specific compiler (e.g icc) and put them
# in directories dedicated to that compiler


driver.o:  $(CBA_DIR)/driver.c $(CBA_DIR)/makefile $(CBA_DIR)/cba.h $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(CBA_DIR) -c $<

cba.o:	$(CBA_DIR)/cba.c $(CBA_DIR)/makefile $(CBA_DIR)/cba.h $(CM_INC)
		success=0 && $(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(CBA_DIR) -c $< && success=1;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

print_cmplr_flags.o: $(CM_DIR)/print_cmplr_flags.c $(CBA_DIR)/makefile cflags.h
		$(CC) $(CFLAGS) -DHAS_CFLAGS -I$(CM_DIR) -I$(CBA_DIR) -c $<


# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation

cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(CBA_DIR) ./driver.c ./cba.c


# Check the output from first run against a known case
check:		./cba
		if \
                     test "$(LAUNCHER)" == "ressub" ; \
                then \
                     echo "./cba 401 7173289  64 64 > cba_401_check_output" > resin ; \
                     /bin/cp ./cba $(RES_WK_DIR) ; \
                     ressub -label cba $(RES_POOL)  $(PLT_LST) -f resin -o cba_401_check_output -e cba_err -work_dir $(RES_WK_DIR) > ressub_out ; \
                     /bin/rm -f $(RES_WK_DIR)/cba ; \
                else \
                     $(LAUNCHER) ./cba 401 7173289  64 64 > cba_401_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 == 2) print $$1 , "checked failed" ; \
                          }' ./cba_401_check_output > tiny_cba_401_diffs ; \
                else \
                     /usr/bin/diff ./cba_401_correct_output ./cba_401_check_output > cba_401_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./cba_401_diffs > tiny_cba_401_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_cba_401_diffs\n"

# Check the output from all runs against a known case
check_all:	./cba
		./cba 401 7173289   64     64  >  cba_all_check_output
		./cba 501  202323  145   2368  >> cba_all_check_output
		./cba 601  3400    231 128000  >> cba_all_check_output
		- /usr/bin/diff ./cba_all_correct_output ./cba_all_check_output > cba_all_diffs
		/bin/egrep -v "END|INFO|INIT|---" ./cba_all_diffs > tiny_cba_all_diffs

.PHONY: install
install:
		/bin/cp ./cba $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(CBA_OBJS) ./*~ ./#*# ./cflags.h ./cba_401_check_output ./cba_all_check_output \
                         ./cba_401_diffs  ./tiny_cba_401_diffs ./cba_all_diffs ./tiny_cba_all_diffs ./*.oo \
                         ./cba_err ./resin ./ressub_out

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./cba ./makefile


TAGS:   ./driver.c ./cba.c ./cba.h
		etags --declararations ./driver.c ./cba.c ./cba.h --include=../support/TAGS
