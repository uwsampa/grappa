# Benchmark TILT

# CVS info
# $Date: 2010/04/08 20:01:08 $
# $Revision: 1.18 $
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

# BASECFLAGS = -I. -I../support

SHELL	= /bin/sh

FARM_ROOT	= ~/your path goes here or set FARM_ROOT in environment
CM_DIR		= $(FARM_ROOT)/support
TILT_DIR	= $(FARM_ROOT)/tilt

RES_DIR		= /usr/local/links/bin
RES_POOL	=
RES_WK_DIR      =

#PLT_LST		= -platform_list yyy   reshosts -sum
#PLT_LST		=  -platform_list x86_64-m5-linux6
PLT_LST         =

#VPATH = $(CM_DIR) $(GTSC_DIR)

#Shortened output conditional compile
OFMT            =
#Unique identifier conditional compile
UID             =

CC		=
CFLAGS		=

SPLINT		= /usr/bin/splint
SPFLAGS		= -weak -posix-lib

# Common routines used by all benchmarks. Error routines, timers, and random numbers
CM_OBJS		= $(CM_DIR)/for_drivers.o $(CM_DIR)/error_routines.o $(CM_DIR)/bm_timers.o \
                  $(CM_DIR)/brand.o $(CM_DIR)/uqid.o

TILT_C_OBJS	= ./tilt.o ./driver.o ./print_cmplr_flags.o
TILT_F_OBJS	= ./ftilt.o ./fdriver.o ./print_cmplr_flags.o

OBJS_C		= $(TILT_C_OBJS) $(CM_OBJS)
OBJS_F		= $(TILT_F_OBJS) $(CM_OBJS)

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
all:		tilt ftilt

# C code tilt
tilt:			$(TILT_C_OBJS) $(TILT_DIR)/tilt.c ./cflags.h $(CM_INC)
			$(CC) $(CFLAGS) -o tilt $(OBJS_C) $(LDFLAGS)

driver.o:		$(TILT_DIR)/driver.c $(TILT_DIR)/makefile $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -DCSEED -I$(CM_DIR) -I$(TILT_DIR) -c $<

tilt.o:			$(TILT_DIR)/tilt.c $(TILT_DIR)/makefile $(CM_INC)
			success=0 && $(CC) $(CFLAGS)  $(OFMT) $(UID) -I$(CM_DIR) -I$(TILT_DIR) -c $< && success=1 ;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error; \
				exit -1; \
			fi

# Fortran90 code ftilt
ftilt:			$(TILT_F_OBJS) $(TILT_DIR)/ftilt.f90 ./cflags.h ./fflags.h $(CM_INC)
			success=0 && $(F90) $(FFLAGS) -o ftilt $(OBJS_F) $(LDFLAGS) && success=1 ;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error; \
				exit -1; \
			fi
				

# To find the symbol name of `ftilt` that the C driver program will need
# run `make ftilt.s` and find the symbol in the assembly
ftilt.s:
			$(F90) -c -S ./ftilt.f90 -o symbol_name
# change -DTILT="" below if needed for proper linking
fdriver.o:		$(TILT_DIR)/driver.c $(TILT_DIR)/makefile $(CM_INC)
			$(CC) $(CFLAGS) $(OFMT) $(UID) -DFSEED -DHAS_CFLAGS -DHAS_FFLAGS -DTILT="tilt_" \
                            -I$(CM_DIR) -I$(TILT_DIR) -c ./driver.c -o fdriver.o

ftilt.o:		$(TILT_DIR)/ftilt.f90 $(TILT_DIR)/makefile $(CM_INC)
			$(F90) $(FFLAGS) -I$(CM_DIR) -I$(TILT_DIR) -c $<


print_cmplr_flags.o:	$(CM_DIR)/print_cmplr_flags.c $(TILT_DIR)/makefile ./cflags.h ./fflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -DHAS_FFLAGS -I$(CM_DIR) -I$(TILT_DIR) -c $<


# Save C compiler and flags in include file cflags.h
cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Save Fortran compiler and flags in include file fflags.h
fflags.h:
		echo "char fflags[] = " \"$(F90) $(FFLAGS) \" \; > fflags.h

# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(TILT_DIR) ./driver.c ./tilt.c

# Check the output against a known case
check:		./tilt ./ftilt
		if \
                     test "$(LAUNCHER)" == "ressub"; \
                then \
                     echo "./tilt  101 119554831 " >  tilt_res_in ; \
                     echo "./ftilt 201 119554831 " > ftilt_res_in ; \
                     /bin/cp ./tilt  $(RES_WK_DIR) ; \
                     /bin/cp ./ftilt $(RES_WK_DIR) ; \
                     ressub -label tilt  $(RES_POOL) $(PLT_LST) -f tilt_res_in -o tilt_101_check_output \
                          -e tilt_err -work_dir $(RES_WK_DIR)  > t_ressub_out ; \
                     ressub -label ftilt $(RES_POOL) $(PLT_LST) -f ftilt_res_in -o ftilt_201_check_output \
                          -e ftilt_err -work_dir $(RES_WK_DIR) > f_ressub_out ; \
                     /bin/rm -f $(RES_WK_DIR)/tilt $(RES_WK_DIR)/ftilt ; \
                else \
                     $(LAUNCHER) ./tilt  101 119554831 > tilt_101_check_output  ; \
                     $(LAUNCHER) ./ftilt 201 119554831 > ftilt_201_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 >  1) print $$1 , "checked failed" ; \
                          }' ./tilt_101_check_output > tiny_tilt_101_diffs ; \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 >  1) print $$1 , "checked failed" ; \
                          }' ./ftilt_201_check_output > tiny_ftilt_201_diffs ; \
                else \
                     /usr/bin/diff ./tilt_101_correct_output ./tilt_101_check_output > tilt_101_diffs    ; \
                     /usr/bin/diff ./ftilt_201_correct_output ./ftilt_201_check_output > ftilt_201_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./tilt_101_diffs > tiny_tilt_101_diffs   ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./ftilt_201_diffs > tiny_ftilt_201_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_tilt_101_diffs and tiny_ftilt_201_diffs\n"

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./tilt $(INSTALL_DIR)
		/bin/cp ./ftilt $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(TILT_C_OBJS) $(TILT_F_OBJS) ./*~ ./#*# ./cflags.h ./fflags.h \
                     ./ftilt.s ./tilt_101_diffs ./ftilt_201_diffs ./tiny_tilt_101_diffs ./tiny_ftilt_201_diffs ./*.oo \
                     ./tilt_res_in ./ftilt_res_in ./tilt_101_check_output ./ftilt_201_check_output ./tilt_err ./ftilt_err \
                     ./t_ressub_out ./f_ressub_out

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./tilt ./ftilt ./makefile ./tilt_101_check_output \
                   ./ftilt_101_check_output ./symbol_name

TAGS:   ./tilt.c ./driver.c  ./ftilt.f
		etags -l c ./tilt.c ./driver.c ./ftilt.f --include=../support/TAGS
