# CVS info  
# $Date: 2005/06/07 18:59:09 $    
# $Revision: 1.1 $
# $RCSfile: Makefile,v $ 
# $Name:  $
#    
#
# STOP! Go to the ./EZThreads/src dir and run make there first!
#
# build order:
#   In the following order:
#       1. do make to create the ./EZThreads/src
#       2. do make to create the BENCH13 ./src
#
# Purpose: Subdirectory Makefile for BENCH13
# 
#   1. builds BENCH13 executable in the ./bin direcotry     
#   2. cleans up the files in the bin and src directory
#   4. check list files in the bin and src directory for review
#   5. provides testrun call to execute BENCH13 with test
#      arguments
#
# build:
#    Example:
#      make clean
#      make
#      make testrun 
#
#


BIN = bench13
BINDIR = ../../bin
INCDIR = ../EZThreads/include
LIBDIR = ../EZThreads/lib

BENCH13 = ${BINDIR}/${BIN}

LIBS = -lpthread -lEZThreads -lmpi

CC = /usr/bin/gcc
CFLAGS = -O3 -D_POSIX_C_SOURCE=199506L -I${INCDIR} -L${LIBDIR}

OBJS = bench13.o

${BENCH13}: ${OBJS}
	${CC} ${CFLAGS} -o ${BENCH13} ${OBJS} ${LIBS}
#	chatr +pd 16M ${BENCH13}

${OBJS}: ${INCDIR}/EZThreads.h

clean:
	rm -f ${OBJS} ${BENCH13} core
	
check:
	ls -l ./*
	cd $(BINDIR); ls -l ./*

testrun:
	${BENCH13} 2 24 27
