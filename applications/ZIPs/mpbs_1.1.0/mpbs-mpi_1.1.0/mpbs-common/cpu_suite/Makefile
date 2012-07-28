CFLAGS = -I. -I../support

ifeq "$(strip $(PE_ENV))" "GNU"
CC = cc
CFLAGS += -O3 -std=c99 
else
ifeq "$(strip $(PE_ENV))" "CRAY"
CC = cc
else
CC = gcc
CFLAGS += -O3 -std=c99
endif
endif


CBA_CC=icc
CBA_CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DUSE_POP3 -I. -I../support 
#CBA_CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -funroll-all-loops -finline-functions -march=native -combine -Winline -msse4a -DB_S_X2="b_s_x2_" -I. -I../support

GT_CC=icc
GT_CFLAGS=-O3 -std=c99 -I. -I../support -xHost

SC_CC=icc
SC_CFLAGS=-O3 -std=c99 -I. -I../support -xHost

SORT_CC=icc
SORT_CFLAGS=-O3 -std=c99 -I. -I../support -xHost

#XBA_CC=icc
#XBA_CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -I. -I../support -xHost
#
#XOR_CC=icc
#XOR_CFLAGS=-O3 -std=c99 -I. -I../support -xHost
#
#SUFFIX=$(shell date "+%b%d.%H%M")
#RESULTS_DIR=./results.$(SUFFIX)

.PHONY: all
all:	bindir bmm cba gt sc sort tilt ftilt xba xor cpurun

.PHONY: bindir
################## Added to support MPBS ###########################
libcpu_suite.a: tilt/tilt.o bmm/bmm.o mmbs2/mmbs2.o 
	ar -r $@ tilt/tilt.o bmm/bmm.o mmbs2/mmbs2.o 

bmm/bmm.o:
	cd bmm; make CC='$(CC)' CFLAGS='$(CFLAGS)'  bmm.o

mmbs2/mmbs2.o:
	cd mmbs2; make CC='$(CC)' CFLAGS='$(CFLAGS) -DVEC_WIDTH=128' mmbs2.o

support/brand.o:
	cd support; make CC='$(CC)' CFLAGS='$(CFLAGS)' brand.o

tilt/tilt.o:
	cd tilt; make CC='$(CC)' CFLAGS='$(CFLAGS)' tilt.o
####################################################################

#bindir:
#	if [ ! -d bin ]; \
#	then \
#		mkdir bin; \
#	fi

bmm::
	cd support; make cpu_clean
	cd support; make CC=$(BMM_CC) CFLAGS='$(BMM_CFLAGS)' UID='$(GLOBAL_UID)'
	cd bmm; make CC=$(BMM_CC) CFLAGS='$(BMM_CFLAGS)' UID='$(GLOBAL_UID)'
	if [ -x bmm/bmm ]; \
	then \
		cp bmm/bmm bin/bmm; \
	fi

cba::
	cd support; make cpu_clean
	cd support; make CC=$(CBA_CC) CFLAGS='$(CBA_CFLAGS)' UID='$(GLOBAL_UID)'
	cd cba; make CC=$(CBA_CC) CFLAGS='$(CBA_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x cba/cba ]; \
	then \
		cp cba/cba bin/cba; \
	fi

gt::
	cd support; make cpu_clean
	cd support; make CC=$(GT_CC) CFLAGS='$(GT_CFLAGS)' UID='$(GLOBAL_UID)'
	cd gtsc; make CC=$(GT_CC) CFLAGS='$(GT_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x gtsc/gt ]; \
	then \
		cp gtsc/gt bin/gt; \
	fi

sc::
	cd support; make cpu_clean
	cd support; make CC=$(SC_CC) CFLAGS='$(SC_CFLAGS)' UID='$(GLOBAL_UID)'
	cd gtsc; make CC=$(SC_CC) CFLAGS='$(SC_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x gtsc/sc ]; \
	then \
		cp gtsc/sc bin/sc; \
	fi


sort::
	cd support; make cpu_clean
	cd support; make CC=$(SORT_CC) CFLAGS='$(SORT_CFLAGS)' UID='$(GLOBAL_UID)'
	cd sort; make CC=$(SORT_CC) CFLAGS='$(SORT_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x sort/a_sort ]; \
	then \
		cp sort/a_sort bin/a_sort; \
	fi

tilt::
	cd support; make cpu_clean
	cd support; make CFLAGS='$(TILT_CFLAGS)' UID='$(GLOBAL_UID)'
	cd tilt;make CFLAGS='$(TILT_CFLAGS)' UID='$(GLOBAL_UID)' \
	   F90=$(TILT_F90) FFLAGS='$(TILT_FFLAGS)' tilt
	if [ -x tilt/tilt ]; \
	then \
		cp tilt/tilt bin/tilt; \
	fi

ftilt::
	cd support; make cpu_clean
	cd support; make CC=$(TILT_CC) CFLAGS='$(TILT_CFLAGS)' UID='$(GLOBAL_UID)'
	cd tilt;make CC=$(TILT_CC) CFLAGS='$(TILT_CFLAGS)' UID='$(GLOBAL_UID)' \
	   F90=$(TILT_F90) FFLAGS='$(TILT_FFLAGS)' ftilt
	if [ -x tilt/ftilt ]; \
	then \
		cp tilt/ftilt bin/ftilt; \
	fi

xba::
	cd support; make cpu_clean
	cd support; make CC=$(XBA_CC) CFLAGS='$(XBA_CFLAGS)' UID='$(GLOBAL_UID)'
	cd xba; make CC=$(XBA_CC) CFLAGS='$(XBA_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x xba/xba ]; \
	then \
		cp xba/xba bin/xba; \
	fi

xor::
	cd support; make cpu_clean
	cd support; make CC=$(XOR_CC) CFLAGS='$(XOR_CFLAGS)' UID='$(GLOBAL_UID)'
	cd xor; make CC=$(XOR_CC) CFLAGS='$(XOR_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x xor/xor ]; \
	then \
		cp xor/xor bin/xor; \
	fi

supportclean support.clean support-clean cleansupport clean-support clean.support::
	cd support;make clean
bmmclean bmm.clean bmm-clean cleanbmm clean-bmm clean.bmm::
	cd bmm;make clean
cbaclean cba.clean cba-clean cleancba clean-cba clean.cba::
	cd cba;make clean
gmpclean gmp.clean gmp-clean cleangmp clean-gmp clean.gmp::
	cd gmp;make clean
gtscclean gtsc.clean gtsc-clean cleangtsc clean-gtsc clean.gtsc::
	cd gtsc;make clean
sortclean sort.clean sort-clean cleansort clean-sort clean.sort::
	cd sort;make clean
tiltclean tilt.clean tilt-clean cleantilt clean-tilt clean.tilt::
	cd tilt;make clean
xbaclean xba.clean xba-clean cleanxba clean-xba clean.xba::
	cd xba;make clean
xorclean xor.clean xor-clean cleanxor clean-xor clean.xor::
	cd xor;make clean
cpurunclean cpurun.clean cpurun-clean cleancpurun clean-cpurun clean.cpurun::
	cd cpurun;make clean

.PHONY: clean
clean:
	cd support; make clean
	cd bmm; make clean
	cd cba; make clean
	cd gtsc; make clean
	cd sort; make clean
	cd tilt; make clean
	cd xba; make clean
	cd xor; make clean
	cd mmbs2; make clean

