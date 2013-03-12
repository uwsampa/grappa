SHELL=/bin/sh
BENCHMARK=is
BENCHMARKU=IS

GRAPPA_HOME?=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')

include $(GRAPPA_HOME)/include.mk
include $(GRAPPA_HOME)/system/Makefile

# include ../config/make.def
# include ../sys/make.common

OBJS = is.o ${COMMON}/c_print_results.o ${COMMON}/c_timers.o

${PROGRAM}: config ${OBJS} $(GRAPPA_HOME)/system/libGrappa.a
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB} $(GRAPPA_HOME)/system/libGrappa.a

.c.o:
	${CCOMPILE} $<

is.o: is.cpp npbparams.h
	

$(GRAPPA_HOME)/system/libGrappa.a: force
	cd $(GRAPPA_HOME)/system; $(MAKE) lib

force: ;

allclean: clean
	pushd $(GRAPPA_HOME)/system; $(MAKE) clean; popd

clean:
	- rm -f *.o *~ mputil*
	- rm -f is npbparams.h core
