SHELL=/bin/sh
BENCHMARK=is
BENCHMARKU=IS

include ../config/make.def

include ../sys/make.common

OBJS = is.o \
       ${COMMON}/c_print_results.o \
       ${COMMON}/c_wtime.o

${PROGRAM}: config ${OBJS}
	${CLINK} -pl ${PROGRAM}.pl ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${C_LIB} -lm -lprand -par

is.o:   is.c
	cc -c -pl ${PROGRAM}.pl is.c -par


clean:
	- rm -f *.o *~ mputil* *.pl ../bin/*.pl ${COMMON}/*.o
	- rm -f npbparams.h core
