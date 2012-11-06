NODES=1
CC=gcc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DUSE_POP3 -DB_S_X2="b_s_x2_"
#GLOBAL_UID = 

BMM_CC=gcc
BMM_CFLAGS=-O3 -std=c99 -I. -I../support 


BS2_CC=gcc
BS2_CFLAGS=-O3 -std=c99 -I. -I../support 
BS2_F90       = gfortran	
BS2_FFLAGS    = -O3 -nofor-main

CBA_CC=gcc
CBA_CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DUSE_POP3 -I. -I../support 

GMP_CC=gcc
GMP_CFLAGS=-O3 -std=c99 -I. -I../support 
GMP_LIBS= /usr/local/lib  
GMP_INCLUDES= /usr/local/include

GT_CC=gcc
GT_CFLAGS=-O3 -std=c99 -I. -I../support 

SC_CC=gcc
SC_CFLAGS=-O3 -std=c99 -I. -I../support 

SORT_CC=gcc
SORT_CFLAGS=-O3 -std=c99 -I. -I../support 

TILT_CC=gcc
TILT_CFLAGS=-O3 -std=c99 -I. -I../support 
TILT_F90       = gfortran
TILT_FFLAGS    = -O3 -nofor-main 

XBA_CC=gcc
XBA_CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -I. -I../support 

XOR_CC=gcc
XOR_CFLAGS=-O3 -std=c99 -I. -I../support 

CPURUN_CC=mpicc
CPURUN_CFLAGS=

SUFFIX=$(shell date "+%b%d.%H%M")
RESULTS_DIR=./results.$(SUFFIX)

.PHONY: all
all:	bindir bmm bs2 cba gmp gt sc sort tilt ftilt xba xor cpurun

.PHONY: bindir
bindir:
	if [ ! -d bin ]; \
	then \
		mkdir bin; \
	fi

bmm::
	cd support; make cpu_clean
	cd support; make CC=$(BMM_CC) CFLAGS='$(BMM_CFLAGS)' UID='$(GLOBAL_UID)'
	cd bmm; make CC=$(BMM_CC) CFLAGS='$(BMM_CFLAGS)' UID='$(GLOBAL_UID)'
	if [ -x bmm/bmm ]; \
	then \
		cp bmm/bmm bin/bmm; \
	fi

bs2::
	cd support; make cpu_clean
	cd support; make CC=$(BS2_CC) CFLAGS='$(BS2_CFLAGS)' UID='$(GLOBAL_UID)'
	cd bs2; make CC=$(BS2_CC) CFLAGS='$(BS2_CFLAGS)' UID='$(GLOBAL_UID)' \
	   F90=$(BS2_F90) FFLAGS='$(BS2_FFLAGS)'
	if [ -x bs2/bs2 ]; \
	then \
		cp bs2/bs2 bin/bs2; \
	fi

cba::
	cd support; make cpu_clean
	cd support; make CC=$(CBA_CC) CFLAGS='$(CBA_CFLAGS)' UID='$(GLOBAL_UID)'
	cd cba; make CC=$(CBA_CC) CFLAGS='$(CBA_CFLAGS)' UID='$(GLOBAL_UID)' 
	if [ -x cba/cba ]; \
	then \
		cp cba/cba bin/cba; \
	fi


gmp::
	cd support; make cpu_clean
	cd support; make CC=$(GMP_CC) CFLAGS='$(GMP_CFLAGS)' UID='$(GLOBAL_UID)'
	cd gmp; make CC=$(GMP_CC) CFLAGS='$(GMP_CFLAGS)' UID='$(GLOBAL_UID)' \
		LIBS=$(GMP_LIBS)  INCLUDES=$(GMP_INCLUDES)
	if [ -x gmp/gmp ]; \
	then \
		cp gmp/gmp bin/gmp; \
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
	cd support; make CC=$(TILT_CC) CFLAGS='$(TILT_CFLAGS)' UID='$(GLOBAL_UID)'
	cd tilt;make CC=$(TILT_CC) CFLAGS='$(TILT_CFLAGS)' UID='$(GLOBAL_UID)' \
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

.PHONY: cpurun
cpurun::
	cd cpurun; make CC=$(CPURUN_CC) CFLAGS='$(CPURUN_CFLAGS)'
	if [ -x cpurun/cpurun ]; \
	then \
		cp cpurun/cpurun bin/cpurun; \
	fi

supportclean support.clean support-clean cleansupport clean-support clean.support::
	cd support;make clean
bmmclean bmm.clean bmm-clean cleanbmm clean-bmm clean.bmm::
	cd bmm;make clean
bs2clean bs2.clean bs2-clean cleanbs2 clean-bs2 clean.bs2::
	cd bs2;make clean
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
	cd bs2; make clean
	cd cba; make clean
	cd gtsc; make clean
	cd sort; make clean
	cd tilt; make clean
	cd xba; make clean
	cd xor; make clean
	cd cpurun; make clean

.PHONY: cleanall
cleanall:
	cd support; make clean
	cd bmm; make clean
	cd bs2; make clean
	cd cba; make clean
	cd gtsc; make clean
	cd sort; make clean
	cd tilt; make clean
	cd xba; make clean
	cd xor; make clean
	cd cpurun; make clean
	/bin/rm ../bin/*

.PHONY: run
run:
	srun -n $(NODES) ../bin/cpurun ../bin $(RESULTS_DIR)
#	mpirun -np $(NODES) bin/cpurun ./bin $(RESULTS_DIR)
# 	aprun -n $(NODES) bin/cpurun ./bin $(RESULTS_DIR)
