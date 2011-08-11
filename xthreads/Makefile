NTHR=8
HYP=NOHYPER
PF=1
UNR=1
DEFINES=-D NTHR=$(NTHR) -D$(HYP) -DUNROLL=$(UNR) -DPREFETCH=$(PF)
CFLAGS = -O2 -std=gnu99 -Wall -Wextra -g $(DEFINES) -fopenmp -lm
LDFLAGS = -L../bench
LDLIBS= -lrt -fopenmp -lgraph
all: jumpthreads yield_bench pchase rand newton mathchase rwalk treap printpar


jumpthreads: greenery_compiler.sh
	cp greenery_compiler.sh jumpthreads
	chmod a+x jumpthreads

yield_bench.o: yield_bench.c yield_unrolled.cunroll

%.cunroll : %.gc jumpthreads
	./jumpthreads $< $@ $(NTHR)

printpar.o: printpar.c printpar_unrolled.cunroll
pchase.o: pchase.c pchase_unrolled.cunroll
mathchase.o: mathchase.c mathchase_unrolled.cunroll
rand.o: rand.c rand_unrolled.cunroll
newton.o: newton.c newton_unrolled.cunroll
rwalk.o: rwalk.c rwalk_unrolled.cunroll
treap.o: treap.c treap_unrolled.cunroll
.PHONY: clean

clean:
	rm -f jumpthreads yield_bench.o yield_bench yield_unrolled.cunroll pchase pchase.o pchase_unrolled.cunroll rand.o rand rand_unrolled.cunroll newton illustration.pdf illustration.eps illustration.ps illustration.dvi illustration.aux mathchase mathchase_unrolled.cunroll rwalk.o mathchase.o rwalk_unrolled.cunroll treap.o treap_unrolled.cunroll

illustration.pdf: illustration.eps
	epstopdf $<

illustration.eps: illustration.ps
	ps2eps $<

illustration.ps: illustration.dvi
	dvips $< -o

illustration.dvi: illustration.tex
	latex $<
