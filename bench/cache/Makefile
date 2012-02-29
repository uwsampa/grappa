#
# Makefile for SoftXMT V0
#
# This provides automatic header dependence discovery, 
# as well as an easiery way to run tests
#
# Filenames:
#  - C++ header files are named <name>.hpp;
#    C header files are named <name>.h
#  - Each .cpp file without a main() can include tests and be used to
#    make a .test. These are ususally named <name>_tests.cpp
#  - Executables are named <name>.exe.
#  - Test executables are named <name>.test. 
#  - Tests are run using MPI.
#
# Other notes: 
#  - $LD is actually a C++ compiler, probably mpic++. Individual files
#    are compiled with $CXX; linking is done with $LD.
#  - default directories and flags should be set in a common
#    include.mk, but actual library includes should be set here.
#
# Steps for use
# 1. Add library dependences to $(LIBRARIES) below.
# 2. Add compiler flags to $(CFLAGS) below.
# 3. Add object file dependences below.
# 4. Set a default target below if you want.
# 4. To build the default target, say "make".
#    To build an executable or test, say "make TARGET=<name>.exe" or "make TARGET=name.test".
#    To build and run a test, say "make TARGET=<name>.test test". 
#    To build and run an executable, say "make TARGET=<name>.test run". 
#

SOFTXMT_HOME:=$(HOME)/softxmt

# include common variable definitions
include $(SOFTXMT_HOME)/include.mk

# name of main executable to build and/or run
TARGET:= cache_experiment.exe

# default rule: just build
$(TARGET): 

CFLAGS+= -I$(SOFTXMT_HOME)/system

cache_experiment.exe: cache_experiment.o $(SOFTXMT_HOME)/system/libSoftXMT.a

test.exe: test.o $(SOFTXMT_HOME)/system/libSoftXMT.a


include $(SOFTXMT_HOME)/system/Makefile
