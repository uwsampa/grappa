# CVS info  
# $Date: 2005/06/07 18:58:35 $    
# $Revision: 1.1 $
# $RCSfile: Makefile,v $ 
# $Name:  $    
#
#
# build order:
#   In the following order:
#       1. do make to create the ./EZThreads/src
#       2. do make to create the bench13 ./src
#
# Purpose: Subdirectory Makefile for ./EZThreads/src
# 
#   1. builds libEZThreads.a in the ./EZThreads/lib direcotry     
#   2. cleans up the files in the ./EZThreads/lib and ./EZThreads/src dirs
#   3. check list files in the bin and src directory for review
#
# build:
#    Example:
#      make clean
#      make
#
#

SHELL = /bin/sh

INCDIR = ../include
LIBDIR = ../lib

#CC = /usr/bin/gcc
CC = cc
CFLAGS = -O3 -D_POSIX_C_SOURCE=199506L -I${INCDIR}

OBJS = EZmisc.o EZpcall.o EZatomicCnt.o EZbarrier.o

libEZThreads.a: ${OBJS}
	ar rv ${LIBDIR}/$@ ${OBJS}
	- ranlib ${LIBDIR}/$@
	rm -f ./*.o
	rm -f ./*.a


${OBJS}: ${INCDIR}/EZThreads.h

check:
	ls -l ./*
	cd $(LIBDIR); ls -l ./*

clean:
	rm -f ./*.[oa]
	rm -f ${LIBDIR}/*.a

