SHELL=/bin/sh
BENCHMARK=is
BENCHMARKU=IS

include ../config/make.def

include ../sys/make.common

OBJS = is.o ${COMMON}/c_print_results.o ${COMMON}/c_timers.o


${PROGRAM}: config ${OBJS}
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB} ../../../system/libGrappa.a

.c.o:
	${CCOMPILE} $<

is.o:             is.cpp  npbparams.h


clean:
	- rm -f *.o *~ mputil*
	- rm -f is npbparams.h core
