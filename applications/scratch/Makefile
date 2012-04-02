#
# Makefile for SoftXMT V0
#
# This provides automatic header dependence discovery, 
# as well as an easiery way to run tests

# include common variable definitions
include $(SOFTXMT_HOME)/include.mk

#
# Define DEBUG to build debug versions of binaries/tests
# This may be done at the command line: "make TARGET=foo.test DEBUG=1 test"
#
#DEBUG:=1

# name of main executable to build and/or run
TARGET:= area_under_curve.exe

# default rule: just build
$(TARGET): 

CFLAGS+= -I$(SOFTXMT_HOME)/system

#
# object file dependences
#
area_under_curve.exe: area_under_curve.o $(SOFTXMT_HOME)/system/libSoftXMT.a

include $(SOFTXMT_HOME)/system/Makefile

