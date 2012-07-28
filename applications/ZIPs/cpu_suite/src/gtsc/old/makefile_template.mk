# Benchmark GT and SC

# CVS info
# $Date: 2010/04/08 19:56:35 $
# $Revision: 1.22 $
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
GTSC_DIR	= $(FARM_ROOT)/gtsc

RES_DIR		= /usr/local/links/bin
#RES_POOL	= -P xxxx
RES_POOL	=
RES_WK_DIR      =

# set PLT_LST in environment
#PLT_LST		= -platform_list yyy   reshosts -sum => x86_64-cpu-linux6
#PLT_LST		= -platform_list x86_64-cpu-linux6

#VPATH = $(CM_DIR) $(GTSC_DIR)

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

GTSC_G_OBJS	= ./sc.o ./driver_gt.o ./print_cmplr_flags.o
GTSC_S_OBJS	= ./gt.o ./driver_sc.o ./print_cmplr_flags.o

OBJS_G		= $(GTSC_G_OBJS) $(CM_OBJS)
OBJS_S		= $(GTSC_S_OBJS) $(CM_OBJS)

# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS  =

CM_INC   = $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bench.h $(CM_DIR)/bm_timers.h \
             $(CM_DIR)/error_routines.h  $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:		gt sc

# Gather

gt:			$(GTSC_G_OBJS) $(GTSC_DIR)/gt.c ./cflags.h $(CM_INC)
			$(CC) $(CFLAGS) -o gt $(OBJS_G)  $(LDFLAGS)
			date

driver_gt.o:		$(GTSC_DIR)/driver_gt.c $(GTSC_DIR)/makefile $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(GTSC_DIR) -c $<

gt.o:			$(GTSC_DIR)/gt.c $(GTSC_DIR)/makefile $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(GTSC_DIR) -c $<

print_cmplr_flags.o:	$(CM_DIR)/print_cmplr_flags.c $(GTSC_DIR)/makefile ./cflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -I$(CM_DIR) -I$(GTSC_DIR) -c $<


# Scatter

sc:		$(GTSC_S_OBJS) $(GTSC_DIR)/sc.c ./cflags.h $(CM_INC)
		$(CC) $(CFLAGS) -o sc $(OBJS_S) $(LDFLAGS)
		date

driver_sc.o:	$(GTSC_DIR)/driver_sc.c $(GTSC_DIR)/makefile $(CM_INC)
		$(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(GTSC_DIR) -c $<

sc.o:		$(GTSC_DIR)/sc.c $(GTSC_DIR)/makefile $(CM_INC)
		success=0 && $(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(GTSC_DIR) -c $< && success=1;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi


# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation

cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(GTSC_DIR) ./sc.c ./driver_gt.c ./gt.c ./driver_sc.c

# Check the output against a known case
check:         ./gt ./sc
		if \
                     test "$(LAUNCHER)" == "ressub"; \
                then \
                     /bin/cp ./gt ./sc $(RES_WK_DIR) ; \
                     echo "./gt  1501  112197   8 " > gt_res_in ; \
                     echo "./sc  1801  75412    8 " > sc_res_in ; \
                     ressub -label gt $(RES_POOL) $(PLT_LST) -f gt_res_in -o gt_1501_check_output \
                        -e gt_err -work_dir $(RES_WK_DIR) > gt_res_out ; \
                      ressub -label sc $(RES_POOL) $(PLT_LST) -f sc_res_in -o sc_1801_check_output \
                        -e sc_err -work_dir $(RES_WK_DIR) > sc_res_out ; \
                     /bin/rm -f $(RES_WK_DIR)/gt $(RES_WK_DIR)/sc ; \
                else \
                     $(LAUNCHER) ./gt  1501  112197   8 > gt_1501_check_output ; \
                     $(LAUNCHER) ./sc  1801  75412    8 > sc_1801_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 >  1) print $$1 , "checked failed" ; \
                          }' ./gt_1501_check_output > tiny_gt_1501_diffs ; \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 >  1) print $$1 , "checked failed" ; \
                          }' ./sc_1801_check_output > tiny_sc_1801_diffs ; \
                else \
                     /usr/bin/diff ./gt_1501_correct_output ./gt_1501_check_output > gt_1501_diffs ; \
                     /usr/bin/diff ./sc_1801_correct_output ./sc_1801_check_output > sc_1801_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./gt_1501_diffs > tiny_gt_1501_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./sc_1801_diffs > tiny_sc_1801_diffs ; \
                fi
		 @ echo -e "\n=======> results in files tiny_gt_1501_diffs and tiny_sc_1801_diffs\n"

check_all:	./gt ./sc
		./gt 1501 112197   8  >  gt_all_check_output
		./gt 1601    552  16  >> gt_all_check_output
		./gt 1701     29  25  >> gt_all_check_output
		./sc 1801  75412   8  >  sc_all_check_output
		./sc 1901    147  16  >> sc_all_check_output
		./sc 2001     11  25  >> sc_all_check_output
		- /usr/bin/diff ./gt_all_correct_output ./gt_all_check_output  > gt_all_diffs
		- /usr/bin/diff ./sc_all_correct_output ./sc_all_check_output  > sc_all_diffs
		/bin/egrep -v "END|INFO|INIT|---" ./gt_all_diff > tiny_gt_all_diff
		/bin/egrep -v "END|INFO|INIT|---" ./sc_all_diffs  > tiny_sc_all_diffs

# Run code under RES
# sec_frm_epoch  returns the number of seconds since the epoch
# option -l is when the RES job is launched
# option -s is the number of second to sleep before timed code is started
check_with_RES:	./gt ./sc
		echo "./gt  1501  112197  -l `$(CM_DIR)/sec_frm_epoch`  -s 60 8 " > gt_res_in
		$(RES_DIR)/ressub $(RES_POOL) $(PLT_LST) -f gt_res_in -o gt_res_stdout -e gt_err > gt_res_out
		echo "./sc  1801  75412   -l `$(CM_DIR)/sec_frm_epoch`  -s 60 8 " > sc_res_in
		$(RES_DIR)/ressub $(RES_POOL) $(PLT_LST) -f sc_res_in -o sc_res_stdout -e sc_err > sc_res_out

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./gt $(INSTALL_DIR)
		/bin/cp ./sc $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(GTSC_G_OBJS) $(GTSC_S_OBJS) ./*~ ./#*# ./cflags.h \
                     ./gt_1501_check_output ./sc_1801_check_output ./gt_1501_diffs ./sc_1801_diffs \
                     ./gt_all_check_output ./sc_all_check_output ./gt_all_diffs ./sc_all_diffs \
                     ./gt_res_stdout ./gt_err ./sc_res_stdout ./sc_err \
                     ./gt_res_out ./sc_res_out ./gt_res_in ./sc_res_in  ./tiny_gt_1501_diffs \
                     ./tiny_sc_1801_diffs ./tiny_gt_all_diff ./tiny_sc_all_diffs ./*.oo

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./gt ./sc ./makefile


TAGS:   ./sc.c ./driver_gt.c ./gt.c ./driver_sc.c
		etags -l c ./sc.c ./driver_gt.c ./gt.c ./driver_sc.c --include=../support/TAGS
