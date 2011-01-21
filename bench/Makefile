CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../ -fopenmp -msse4.1
LDFLAGS= -L../greenery
LDLIBS= -lgreenery -lrt -fopenmp -lnuma
APPS= treegen erdos
TESTS=
MYLIBS= libgraph.a
BENCHES= treesum treedepth matmul
RESULTS=$(APPS) $(TESTS) $(BENCHES) $(MYLIBS)
# TODO: make this makefile make sense 
SRCS= graph.c treegen.c treesum.c treedepth.c matmul.c erdos.c

.PHONY: all clean

all: $(RESULTS)

treedepth: treedepth.o $(MYLIBS)
treegen: treegen.o $(MYLIBS)
treesum: treesum.o $(MYLIBS)
matmul: matmul.o $(MYLIBS)
erdos: erdos.o $(MYLIBS)

libgraph.a: libgraph.a(graph.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
