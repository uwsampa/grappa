###############################################################################
#
#     Makefile for the Massive Parallel Bucket Sort(MPBS) system.
#
#     This makefile produces the mpbs-mpi1 and mpbs-mpi2 executables.
#     The default build is for an SGI or Cray. If you are
#     building on a system that uses mpicc for compilation, then run with
#     PLATFORM=GENERIC.  To build both binaries, you must run make mpi1 
#     and make mpi2.  make without a target will build the mpi1 version only.
#
#     NOTES: 
#          1. Direct IO is only supported on the Cray.
#          2. The MMBS2 kernel on Cray is only supported with the PrgEnv-gnu
#             module loaded.
#
#     Authors: EAH, RLB
#
#     Variable descriptions for the typically modified variables:
#	CC         Compiler
#       CFLAGS     Flags for the compiler (optimizations, warnings, etc...)
#       INCLUDES   Additional include paths
#       LDFLAGS    Additional paths for the linker (-L <path>)
#       LIBS       Libraries to be added
#
#     Target descriptions:
#     mpi1:            Builds the MPI-1 version of mpbs.
#     mpi2:            Builds the MPI-2 version of mpbs.
#     clean:           Removes executables and object files.
#
###############################################################################

NAME            = mpbs
VERSION	        = \"1.1.0\"
TAR_VERSION     = 1.1.0

PLATFORM        = SGI
ifdef PE_ENV
PLATFORM        = CRAY
endif

export PLATFORM

#-------------------------- Directories ---------------------------------------
BLD	        = build
VARIANT         = mpi1
BUILD           = $(BLD)/$(VARIANT)
SRC_SPECIFIC    = mpbs-specific
SRC_COMMON      = mpbs-common
UTILS           = bct-utils
INCS 	       := $(SRC)/include $(SRC_COMMON)/include $(UTILS)/include
SRCU           := $(SRC_COMMON)/usrt
SRCCPU_SUITE   := $(SRC_COMMON)/cpu_suite
LIB             = lib

#-------------------------- Executables ---------------------------------------
EXE             = $(NAME)-$(VARIANT)

#-------------------------- Libraries -----------------------------------------
LIBUSRT        := $(LIB)/libusort.a
LIBCPU_SUITE   := $(LIB)/libcpu_suite.a

#-------------------------- Modules -------------------------------------------
MODULES     = bct-utils/timer \
              bct-utils/rand64 \
              bct-utils/hpc_output \
              bct-utils/omp_utils \
              mpbs-specific \
              mpbs-common

# Dependency compiler
MAKEDEPC    = gcc -MM

CC          = cc
CFLAGS      = -O3 -std=c99 -mpopcnt -fopenmp
INCFLAGS   := $(patsubst %,-I%,$(INCS))
DEFS       := -DPACKAGE_VERSION=$(VERSION)
LDFLAGS    := -L$(LIB) 
LIBS        = -lmpi -lm -lgmp
SRC         =

ifeq ($(strip $(PLATFORM)),GENERIC)
CC          = mpicc
LIBS        = -lm -lgmp
endif

ifeq ($(strip $(MAKECMDGOALS)),)
DEFS    += -DMPI
VARIANT  = mpi1
mpi1: $(EXE)
else
ifeq ($(strip $(MAKECMDGOALS)),mpi1)
DEFS    += -DMPI
VARIANT  = mpi1
mpi1: $(EXE)
else
ifeq ($(strip $(MAKECMDGOALS)),mpi2)
DEFS    += -DMPI2
VARIANT  = mpi2
mpi2: $(EXE)
endif
endif
endif

# After the platform specific initial libraries are set, we can include each
# module's SRC and LIBS needs:
include $(patsubst %, %/module.mk, $(MODULES))

OBJS       := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(filter %.c,$(SRC))))

#-------------------------- Cray specific modifications -----------------------
ifeq "$(strip $(PLATFORM))" "CRAY"
# Requires Cray programming environment
CC          = cc
CFLAGS      = -O3 
LDFLAGS    := -L$(LIB)
LIBS        = -lgmp

ifeq "$(strip $(PE_ENV))" "GNU"
CFLAGS     += -std=c99 -fopenmp 
LIBS       += -lm
endif
endif

#--------------------------- Misc ---------------------------------------------
.PHONY: setup dist clean cleanall cleandist doc

.SUFFIXES:
.SUFFIXES: .o .c .h .a

# places to search for various files
vpath %.c $(MODULES)
vpath %.h $(INCS)
vpath %.a $(LIB)


#--------------------------- Executables -------------------------------------
$(EXE): $(OBJS) $(LIBUSRT) $(LIBCPU_SUITE)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS) 


#-------------------------- Object file creation -----------------------------

ifeq ($(strip $(filter dist clean%, $(MAKECMDGOALS)) ),)
-include $(OBJS:.o=.d)
endif

$(BUILD)/%.d: %.c
	@if [ ! -d $(BUILD)  ]; then mkdir -p $(BUILD); fi 
	@echo "Generating dependencies for $<"
	@$(MAKEDEPC) $(DEFS) $(INCFLAGS) $< | \
        sed 's@^\(.*\)\.o:@$(BUILD)/\1.d $(BUILD)/\1.o:@g' > $@

$(BUILD)/%.o: %.c
	@if [ ! -d $(BUILD)  ]; then mkdir -p $(BUILD); fi
	$(CC) -o $@ -c $(CFLAGS) $(INCFLAGS) $(DEFS) $< 

#-------------------------- library file creation ----------------------------
$(LIBUSRT):
	@if [ ! -d $(LIB) ]; then mkdir -p $(LIB); fi
	cd $(SRCU);          \
        make -f Makefile;    \
        mv $(notdir $@) ../../$(LIB)

$(LIBCPU_SUITE):
	@if [ ! -d $(LIB) ]; then mkdir -p $(LIB); fi
	cd $(SRCCPU_SUITE);          \
        make libcpu_suite.a;    \
        mv $(notdir $@) ../../$(LIB)

#-------------------------- tar ----------------------------------------------
DIST_FILES = AUTHORS Makefile  README COPYRIGHT
DIST_DIRS  = bct-utils mpbs-common mpbs-specific
TAR_DIR    = $(NAME)-mpi_$(TAR_VERSION)
am__tar    = tar chof - "$$tardir"
tar__excl  = --exclude-vcs --exclude='*.svn'--exclude=$(BUILD) --exclude=$(EXE)

am__remove_tardir =                                                  \
  { test ! -d $(TAR_DIR)                                             \
    || { find $(TAR_DIR) -type d ! -perm -200 -exec chmod u+w {} ';' \
         && rm -fr $(TAR_DIR); }; }


dist: 
	test -d $(TAR_DIR) || mkdir $(TAR_DIR)
	list='$(DIST_FILES)';                        \
           for file in $$list; do                    \
              echo $$file; cp $$file $(TAR_DIR);     \
           done
	list='$(DIST_DIRS)';                         \
            for dir in $$list; do                    \
               echo $$dir; cp -a $$dir $(TAR_DIR);   \
            done

dist-gzip: dist
	tardir=$(TAR_DIR) && $(am__tar) $(tar__excl) | gzip -c >$(TAR_DIR).tar.gz
	$(am__remove_tardir)

dist-bzip2: dist
	tardir=$(TAR_DIR) && $(am__tar) $(tar__excl) | bzip2 -9 -c >$(TAR_DIR).tar.bz2
	$(am__remove_tardir)

#-------------------------- documentation ------------------------------------
doc:
	doxygen Doxyfile
	cd doc/latex && make && pdflatex refman


#-------------------------- clean up -----------------------------------------
cleanall: clean cleanrun cleancpu_suite

clean:
	@echo Cleaning entire distribution...
	@rm -rf $(BLD)/*/*.o mpbs-mpi1 mpbs-mpi2 $(LIB)
	@cd $(SRCU); make -f Makefile clean

cleancpu_suite: 
	@echo Cleaning CPU suite...
	@cd $(SRCCPU_SUITE); make clean

cleanrun: 
	@rm -rf *.dat sorted
