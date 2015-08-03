#############################################################################
# Grappa GNU Make include file
#
# This sets variables such that key Grappa-related flags and paths are
# avaiable to makefiles than include this file.
#
# To use this:
#
#   1. Make sure GRAPPA_PREFIX contains the path to your Grappa
#   installation directory. If the default value set in this file is
#   not correct, override it in your Makefile or by setting an
#   environment variable.
#
#   2. Include this file in your Makefile with a line like this:
#      include ${CMAKE_INSTALL_PREFIX}/share/Grappa/grappa.mk
#
#   3. The following variables are then avaliable to you in writing make rules:
#      $(GRAPPA_CXX)
#      $(GRAPPA_CC)
#      $(GRAPPA_LD)
#      $(GRAPPA_CXXFLAGS)
#      $(GRAPPA_LDFLAGS)
#      $(GRAPPA_LDLIBS)
#
# You may use GNU Make's implicit rules to compile Grappa programs by
# setting the GRAPPA_IMPLICIT_RULES before including this file, like
# this:
#      GRAPPA_IMPLICIT_RULES:=on
#      include ${CMAKE_INSTALL_PREFIX}/share/Grappa/grappa.mk
# Then building a Grappa app is as simple as adding a rule like this:
#      grappa_app: grappa_app.o
#
# Note that it is not necessary to use "mpicxx" or other wrappers when
# using the variables set by this file to build Grappa programs; the
# libraries and flags necessary to build MPI code are included in
# these variables by the install process.
#############################################################################

#############################################################################
# original Grappa install location
#############################################################################
ifndef GRAPPA_PREFIX
GRAPPA_PREFIX:=${CMAKE_INSTALL_PREFIX}
endif

#############################################################################
# compiler used to build Grappa
#############################################################################

# MPI header paths should automatically be included in CXXFLAGS by
# CMake, so use the plain C++ compiler (but mpicxx should be fine too)
GRAPPA_CXX=${CMAKE_CXX_COMPILER}

# MPI libraries should automatically be included in CXXFLAGS by
# CMake, so use the plain C++ compiler (but mpicxx should be fine too)
GRAPPA_CC=${CMAKE_CXX_COMPILER}
GRAPPA_LD=${CMAKE_CXX_COMPILER}

#############################################################################
# compilation flags
#############################################################################

GRAPPA_COMPILE_FLAGS=${GMAKE_CXX_DEFINES} ${GMAKE_CXX_FLAGS}
GRAPPA_INCLUDE_PATHS=-I$(GRAPPA_PREFIX)/include -I$(GRAPPA_PREFIX)/include/Grappa -I$(GRAPPA_PREFIX)/include/Grappa/tasks ${GMAKE_CXX_INCLUDE_DIRS_INSTALLED}

# assign to CXXFLAGS for automatic builds using gmake default implicit rules
GRAPPA_CXXFLAGS=$(GRAPPA_COMPILE_FLAGS) $(GRAPPA_INCLUDE_PATHS)

#############################################################################
# link flags
#############################################################################

GRAPPA_LINK_FLAGS=${GMAKE_LINK_FLAGS}
GRAPPA_LINK_PATHS=-L$(GRAPPA_PREFIX)/lib ${GMAKE_LIB_PATHS_INSTALLED}

# assign to LDFLAGS for automatic builds using gmake default implicit rules
GRAPPA_LDFLAGS=$(GRAPPA_LINK_FLAGS) $(GRAPPA_LINK_PATHS)

# assign to LDLIBS for automatic builds using gmake default implicit rules
GRAPPA_LDLIBS=${CMAKE_EXE_LINK_STATIC_CXX_FLAGS} ${GMAKE_STATIC_LIBS} \
	${CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS} ${GMAKE_DYNAMIC_LIBS}

#############################################################################
# support for implicit make rules
#############################################################################

ifdef GRAPPA_IMPLICIT_RULES
CXX=$(GRAPPA_CXX)
CC=$(GRAPPA_CC)
LD=$(GRAPPA_LD)
CXXFLAGS=$(GRAPPA_CXXFLAGS)
LDFLAGS=$(GRAPPA_LDFLAGS)
LDLIBS=$(GRAPPA_LDLIBS)
endif
