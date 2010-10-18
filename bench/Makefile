CFLAGS = -g -std=c99 -Wall -Wextra

APPS= treegen
TESTS= treetest
LIBS= libtree.a
BENCHES=

RESULTS=$(APPS) $(TESTS) $(BENCHES) $(LIBS)

SRCS= treetest.c tree.c treegen.c

.PHONY: all clean

all: $(RESULTS)

treetest: treetest.o $(LIBS)
treegen: treegen.o $(LIBS)

libtree.a: libtree.a(tree.o)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
