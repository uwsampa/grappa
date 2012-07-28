# CVS info  
# $Date: 2005/06/07 18:57:15 $    
# $Revision: 1.1 $
# $RCSfile: Makefile,v $ 
# $Name:  $
#
# Purpose: Toplevel Makefile for bench13
# 
#   1. calls the subdirectory Makefiles in ./EZThreads/src and ./src
#      which build and cleans up the EZThread archive lib and 
#      the bench13 executable
#   2. list files review
#   3. provides testrun call to execute bench13 with test
#      arguments
#   4. copy README file to srcdir and bindir for easy access
#
#
# Toplevel build:
#    Example:
#      make clean
#      make
#      make testrun 
#
# Subdirectory build:
#   In the following order:
#       1. do make to create the ./EZThreads/src 
#       2. do make to create the bench13 ./src
#
#
#
 
EZTSRCDIR = ./EZThreads/src
INCDIR = ./EZThreads/include
LIBDIR = ./EZThreads/lib
BINDIR = ../bin
SRCDIR = ./src


all:
	cd $(EZTSRCDIR); make
	cd $(SRCDIR); make
	
readme:
	cp -p README $(SRCDIR)
	cp -p README $(BINDIR)
			
check:
	ls -l .
	ls -l ./*


clean:
	cd $(EZTSRCDIR); make clean
	cd $(SRCDIR);    make clean
	
cleanall: clean
	rm -f ../bin/*	
run:
	mpirun -np 2 ${BINDIR}/bench13 2 24 27

