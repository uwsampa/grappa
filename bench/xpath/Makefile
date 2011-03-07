CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../.. -fopenmp -msse4.1 -I.. -g
LDFLAGS= -L../../greenery -L..
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs -lgraph
BENCHES= xpath

RESULTS= $(BENCHES)

SRCS=  xpath.c

.PHONY: all clean

all: $(RESULTS)

xpath_unrolled1.cunroll: xpath_unrolled.gc
	./greenery_compiler xpath_unrolled.gc xpath_unrolled1.cunroll 1

xpath_unrolled2.cunroll: xpath_unrolled.gc
	./greenery_compiler xpath_unrolled.gc xpath_unrolled2.cunroll 2
xpath_unrolled4.cunroll: xpath_unrolled.gc
	./greenery_compiler xpath_unrolled.gc xpath_unrolled4.cunroll 4
xpath_unrolled8.cunroll: xpath_unrolled.gc
	./greenery_compiler xpath_unrolled.gc xpath_unrolled8.cunroll 8
xpath_unrolled16.cunroll: xpath_unrolled.gc
	./greenery_compiler xpath_unrolled.gc xpath_unrolled16.cunroll 16

xpath: xpath.c xpath_unrolled1.cunroll xpath_unrolled2.cunroll xpath_unrolled4.cunroll xpath_unrolled8.cunroll xpath_unrolled16.cunroll

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
