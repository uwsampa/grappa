CFLAGS = -g -std=c99 -Wall -Wextra -I../
LDFLAGS= -L../greenery
LDLIBS= -lgreenery
APPS= treegen
TESTS=
MYLIBS= libgraph.a
BENCHES= treesum

RESULTS=$(APPS) $(TESTS) $(BENCHES) $(MYLIBS)

SRCS= graph.c treegen.c treesum.c

.PHONY: all clean

all: $(RESULTS)

treegen: treegen.o $(MYLIBS)
treesum: treesum.o $(MYLIBS)

libgraph.a: libgraph.a(graph.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
