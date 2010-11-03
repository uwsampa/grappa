CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../
LDFLAGS= -L../greenery
LDLIBS= -lgreenery -lrt
APPS= treegen
TESTS=
MYLIBS= libgraph.a
BENCHES= treesum treedepth

RESULTS=$(APPS) $(TESTS) $(BENCHES) $(MYLIBS)

SRCS= graph.c treegen.c treesum.c treedepth.c

.PHONY: all clean

all: $(RESULTS)

treedepth: treedepth.o $(MYLIBS)
treegen: treegen.o $(MYLIBS)
treesum: treesum.o $(MYLIBS)

libgraph.a: libgraph.a(graph.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
