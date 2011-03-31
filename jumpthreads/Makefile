NTHR=8
CFLAGS = -O2 -std=gnu99 -Wall -Wextra -g -D NTHR=$(NTHR) -fopenmp -lm
LDLIBS= -lrt -fopenmp
all: jumpthreads yield_bench pchase rand newton

jumpthreads: greenery_compiler.sh
	cp greenery_compiler.sh jumpthreads
	chmod a+x jumpthreads

yield_bench.o: yield_bench.c yield_unrolled.cunroll

%.cunroll : %.gc jumpthreads
	./jumpthreads $< $@ $(NTHR)

pchase.o: pchase.c pchase_unrolled.cunroll

rand.o: rand.c rand_unrolled.cunroll
newton.o: newton.c newton_unrolled.cunroll
.PHONY: clean

clean:
	rm -f jumpthreads yield_bench.o yield_bench yield_unrolled.cunroll pchase pchase.o pchase_unrolled.cunroll rand.o rand rand_unrolled.cunroll newton
