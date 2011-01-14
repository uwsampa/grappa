CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../.. -fopenmp -msse4.1 -I..
LDFLAGS= -L../../greenery -L..
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs -lgraph
BENCHES= bfs

RESULTS= $(BENCHES)

SRCS=  bfs.c

.PHONY: all clean

all: $(RESULTS)

carlson: bfs.o $(MYLIBS)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
