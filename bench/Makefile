CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../ -fopenmp -msse4.1
LDFLAGS= -L../greenery -fopenmp
LDLIBS= -lgreenery -lrt
APPS= treegen
TESTS=
MYLIBS= libgraph.a
BENCHES= treesum treedepth linked_list

RESULTS=$(APPS) $(TESTS) $(BENCHES) $(MYLIBS)

SRCS= graph.c treegen.c treesum.c treedepth.c

.PHONY: all clean

all: $(RESULTS)

treedepth: treedepth.o $(MYLIBS)
treegen: treegen.o $(MYLIBS)
treesum: treesum.o $(MYLIBS)

linked_list: linked_list.o linked_list-node.o linked_list-alloc.o linked_list-walk.o $(MYLIBS) 

libgraph.a: libgraph.a(graph.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
