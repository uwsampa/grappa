CFLAGS = -O2 -std=gnu99 -Wall -Wextra -I../.. -fopenmp -msse4.1 -I.. -g
LDFLAGS= -L../../greenery -L..
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs -lgraph
BENCHES= xpath
GREENERY= ../../jumpthreads/jumpthreads
RESULTS= $(BENCHES)

SRCS=  xpath.c

.PHONY: all clean

all: $(RESULTS)

#xpath_unrolled1.cunroll: xpath_unrolled.gc
#	$(GREENERY) xpath_unrolled.gc xpath_unrolled1.cunroll 1

#xpath_unrolled2.cunroll: xpath_unrolled.gc
#	$(GREENERY) xpath_unrolled.gc xpath_unrolled2.cunroll 2
#xpath_unrolled4.cunroll: xpath_unrolled.gc
#	$(GREENERY) xpath_unrolled.gc xpath_unrolled4.cunroll 4
#xpath_unrolled8.cunroll: xpath_unrolled.gc
#	$(GREENERY) xpath_unrolled.gc xpath_unrolled8.cunroll 8
#xpath_unrolled16.cunroll: xpath_unrolled.gc
#	$(GREENERY) xpath_unrolled.gc xpath_unrolled16.cunroll 16

xpath_unrolled%.cunroll: xpath_unrolled.gc
	$(GREENERY) xpath_unrolled.gc $@ $*

xpath.o: xpath.c xpath_unrolled1.cunroll xpath_unrolled2.cunroll xpath_unrolled3.cunroll xpath_unrolled4.cunroll xpath_unrolled5.cunroll xpath_unrolled6.cunroll xpath_unrolled7.cunroll xpath_unrolled8.cunroll xpath_unrolled9.cunroll xpath_unrolled10.cunroll xpath_unrolled11.cunroll xpath_unrolled12.cunroll xpath_unrolled13.cunroll xpath_unrolled14.cunroll xpath_unrolled15.cunroll xpath_unrolled16.cunroll

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o) xpath_unrolled1.cunroll xpath_unrolled2.cunroll xpath_unrolled3.cunroll xpath_unrolled4.cunroll xpath_unrolled5.cunroll xpath_unrolled6.cunroll xpath_unrolled7.cunroll xpath_unrolled8.cunroll xpath_unrolled9.cunroll xpath_unrolled10.cunroll xpath_unrolled11.cunroll xpath_unrolled12.cunroll xpath_unrolled13.cunroll xpath_unrolled14.cunroll xpath_unrolled15.cunroll xpath_unrolled16.cunroll
