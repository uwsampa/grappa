CFLAGS = -g -std=c99 -Wall -Wextra -I../
LDFLAGS= -L../greenery
LDLIBS= -lgreenery
APPS= treegen
TESTS= treetest
MYLIBS= libtree.a
BENCHES= treesum

RESULTS=$(APPS) $(TESTS) $(BENCHES) $(MYLIBS)

SRCS= treetest.c tree.c treegen.c treesum.c

.PHONY: all clean

all: $(RESULTS)

treetest: treetest.o $(MYLIBS)
treegen: treegen.o $(MYLIBS)
treesum: treesum.o $(MYLIBS)

libtree.a: libtree.a(tree.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
