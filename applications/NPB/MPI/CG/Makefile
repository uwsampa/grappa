SHELL=/bin/sh
BENCHMARK=cg
BENCHMARKU=CG

include ../config/make.def

OBJS = cg.o ${COMMON}/print_results.o  \
       ${COMMON}/${RAND}.o ${COMMON}/timers.o

include ../sys/make.common

${PROGRAM}: config ${OBJS}
	${FLINK} ${FLINKFLAGS} -o ${PROGRAM} ${OBJS} ${FMPI_LIB}

cg.o:		cg.f  mpinpb.h npbparams.h timing.h
	${FCOMPILE} cg.f


MPITYPE?=SRUN
NNODE?=2
PPN?=1
SRUN_PARTITION?=grappa
SRUN_HOST?=--partition $(SRUN_PARTITION)
SRUN_NPROC=--nodes=$(NNODE) --ntasks-per-node=$(PPN)
SRUN_HOST=--partition $(SRUN_PARTITION)
SRUN_RUN=srun --resv-ports --cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit $(SRUN_FLAGS) $(SRUN_HOST) $(SRUN_NPROC)

run: $(TARGET)
	$(SRUN_RUN) ./$< 

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h core



