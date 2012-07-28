# Benchmark GMP

# CVS info
# $Date: 2010/04/08 19:55:47 $
# $Revision: 1.23 $
# $RCSfile: makefile_template.mk,v $
# $State: Stab $:


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

# edit lib and include path as needed for GMP
# BASECFLAGS = -I. -I../support -I/usr/local/include/GMP

SHELL		= /bin/sh

FARM_ROOT	= ~/your path goes here or set FARM_ROOT in environment
CM_DIR		= $(FARM_ROOT)/support
GMP_DIR		= $(FARM_ROOT)/gmp

LIBS		= /usr/local/lib/GMP
INCLS		= /usr/local/include/GMP

#VPATH = $(CM_DIR) $(GMP_DIR)

RES_DIR		= /usr/local/links/bin
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

GMP_OBJS = ./driver.o ./print_cmplr_flags.o

OBJS = $(GMP_OBJS) $(CM_OBJS)

# LAUNCHER = aprun -q -n 1
# LAUNCHER = prun -n 1
# LAUNCHER = ressub
LAUNCHER =
LDFLAGS  =

CM_INC   = $(CM_DIR)/common_inc.h $(CM_DIR)/local_types.h $(CM_DIR)/bench.h $(CM_DIR)/bm_timers.h \
             $(CM_DIR)/error_routines.h  $(CM_DIR)/brand.h $(CM_DIR)/iobmversion.h

.PHONY: all
all:		gmp

gmp:		$(GMP_OBJS) ./cflags.h $(CM_INC)
		$(CC) $(CFLAGS) $(OFMT) $(UID) -o gmp $(OBJS)  $(LDFLAGS)\
                   -L$(LIBS)  -lgmp
		date

driver.o:	$(GMP_DIR)/driver.c $(GMP_DIR)/makefile $(CM_INC)
		success=0 && $(CC) $(CFLAGS) $(OFMT) $(UID) -I$(CM_DIR) -I$(GMP_DIR) -I$(INCLS) -c $< && success=1;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi

print_cmplr_flags.o: $(CM_DIR)/print_cmplr_flags.c $(GMP_DIR)/makefile ./cflags.h
		$(CC) $(CFLAGS) -DHAS_CFLAGS -I$(CM_DIR) -I$(GMP_DIR) -c $<


# Create a header file that contains a C character array declaration. The
# array contains the compiler name and the compiler flags. This
# is printed by the benchmark as documentation

cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS) -I$(CM_DIR) -I$(GMP_DIR) -I$(INCLS) ./driver.c


# Check the output against a known case
check:		./gmp
		if \
                     test "$(LAUNCHER)" == "ressub" ; \
                then \
                     echo "./gmp 701 25 8192 " > gmp_res_in ; \
                     /bin/cp ./gmp $(RES_WK_DIR) ; \
                     ressub -label gmp $(RES_POOL) $(PLT_LST) -f gmp_res_in -o gmp_701_check_output \
                        -e gmp_err -work_dir $(RES_WK_DIR) > ressub_out ; \
                     /bin/rm -f $(RES_WK_DIR)/gmp ; \
                 else \
                     $(LAUNCHER) ./gmp 701 25 8192 > gmp_701_check_output ; \
                fi
		if \
                     test "$(OFMT)" == "-D__BMK_SHORT_FORMAT" ; \
                then \
                     awk '{ if ( $$3 == 0) print $$1 , "unchecked" ; \
                            if ( $$3 == 1) print $$1 , "checked ok" ; \
                            if ( $$3 == 2) print $$1 , "checked failed" ; \
                          }' ./gmp_701_check_output > tiny_gmp_701_diffs ; \
                else \
                     /usr/bin/diff ./gmp_701_correct_output ./gmp_701_check_output > gmp_701_diffs ; \
                     /bin/egrep -v "END|INFO|INIT|---" ./gmp_701_diffs > tiny_gmp_701_diffs ; \
                fi
		@ echo -e "\n=======> results in files tiny_gmp_701_diffs\n"

check_all:	./gmp
		./gmp   701     25 8192 >  gmp_all_check_output
		./gmp   801    174 4096 >> gmp_all_check_output
		./gmp   901   1179 2048 >> gmp_all_check_output
		./gmp  1001   7340 1024 >> gmp_all_check_output
		./gmp  1101  47533  512 >> gmp_all_check_output
		./gmp  1201 239000  256 >> gmp_all_check_output
		./gmp  1301 992850  128 >> gmp_all_check_output
		- /usr/bin/diff ./gmp_all_correct_output ./gmp_all_check_output > gmp_all_diffs
		/bin/egrep -v "END|INFO|INIT|---" ./gmp_all_diffs > tiny_gmp_all_diffs

.PHONY: install
install:
		if [ ! -d "$(INSTALL_DIR)" ] ; \
                then \
                    mkdir "$(INSTALL_DIR)" ; \
		fi
		/bin/cp ./gmp $(INSTALL_DIR)

.PHONY: clean
clean:
		/bin/rm -f $(GMP_OBJS) ./*~ ./#*# ./cflags.h  ./tiny_gmp_701_diffs ./gmp_701_diffs \
                    ./gmp_701_check_output ./gmp_all_check_output ./gmp_all_diffs  ./tiny_gmp_all_diffs ./*.oo \
                    ./gmp_res_in ./gmp_err ./ressub_out

.PHONY: clobber
clobber:	clean
		/bin/rm -f ./gmp ./makefile


# Remove write permission for safety
files_safer:
		chmod ugo-w $(GMP_DIR)/driver.c $(GMP_DIR)/print_cmplr_flags.c $(GMP_DIR)/makefile_template.mk


TAGS:   ./driver.c
		etags --declarations -l c ./driver.c --include=../support/TAGS
