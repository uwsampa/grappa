CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../.. -fopenmp -msse4.1 -I.. -g
LDFLAGS= -L../../greenery -L..
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs -lgraph
BENCHES= xpath

RESULTS= $(BENCHES)

SRCS=  xpath.c

.PHONY: all clean

all: $(RESULTS)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
