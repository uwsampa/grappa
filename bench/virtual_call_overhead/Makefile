#echo "g++ -g -O3 -Wall -Winline ./virtover.cpp -lrt -o virtover && objdump -S virtover > virtover.dis && ./virtover | sort" | ssh n71.sampa

CXXFLAGS= -g -O3 -Wall -Winline
LDFLAGS=-lrt

run: virtual_call_overhead virtual_call_overhead.dis
	./$< | tee virtual_call_overhead.out
	echo -----------------------------------------------------------------------------
	sort virtual_call_overhead.out
#	./$< | sort

virtual_call_overhead: virtual_call_overhead.cpp

virtual_call_overhead.dis: virtual_call_overhead
	objdump -S $< > $@