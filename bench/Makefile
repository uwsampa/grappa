CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../ -fopenmp -msse4.1
LDFLAGS= -L../greenery
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs
APPS= treegen
TESTS=
MYLIBS= libgraph.a
BENCHES= treesum treedepth linked_list matmul carlson

RESULTS=$(APPS) $(TESTS) $(BENCHES) $(MYLIBS)
# TODO: make this makefile make sense 
SRCS= graph.c treegen.c treesum.c treedepth.c matmul.c carlson.c linked_list.c \
linked_list-alloc.c linked_list-walk.c linked_list-node.c

.PHONY: all clean

all: $(RESULTS)

treedepth: treedepth.o $(MYLIBS)
treegen: treegen.o $(MYLIBS)
treesum: treesum.o $(MYLIBS)
matmul: matmul.o $(MYLIBS)
carlson: carlson.o $(MYLIBS)

linked_list: linked_list.o linked_list-node.o linked_list-alloc.o linked_list-walk.o $(MYLIBS) 

libgraph.a: libgraph.a(graph.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
