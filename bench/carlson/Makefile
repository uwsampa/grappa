CFLAGS = -O2 -std=c99 -Wall -Wextra -I../.. -fopenmp -msse4.1
LDFLAGS= -L../../greenery
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs
BENCHES= carlson

RESULTS= $(BENCHES)

SRCS=  carlson.c

.PHONY: all clean

all: $(RESULTS)

carlson: carlson.o $(MYLIBS)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
