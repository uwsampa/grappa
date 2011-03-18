NTHR=8
CFLAGS = -O2 -std=gnu99 -Wall -Wextra -g -D N_YIELD_THREADS=$(NTHR)
LDLIBS= -lrt
all: jumpthreads yield_bench

jumpthreads: greenery_compiler.sh
	sed -e 's#LOCATION=REPLACE#LOCATION=~/softxmt/jumpthreads#' greenery_compiler.sh > jumpthreads
	chmod a+x jumpthreads

yield_bench.o: yield_bench.c yield_unrolled.cunroll

yield_unrolled.cunroll: yield_unrolled.gc jumpthreads
	./jumpthreads yield_unrolled.gc yield_unrolled.cunroll $(NTHR)

.PHONY: clean

clean:
	rm -f jumpthreads yield_bench.o yield_bench yield_unrolled.cunroll
