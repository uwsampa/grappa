# UNCLASSIFIED//FOR OFFICIAL USE ONLY

# This makefile requires GNU make.  It's a work in progress;
# I don't know how autoconfiguration should be done.

CC=gcc

CFLAGS= -O 

ifeq "$(strip $(PE_ENV))" "GNU"
CC=cc
CFLAGS += -m64 -std=gnu99
else	
ifeq "$(strip $(PE_ENV))" "CRAY"
CC=cc
else
CFLAGS += -m64 -std=gnu99
endif
endif


CACHEMB:=$(shell awk -f cachemb.awk /proc/cpuinfo)

ifndef CACHEMB
nothing:
	@echo usort: Unknown platform $(PLATFORM)
	exit 1
endif

CFLAGS += -I.. -DACTIMER_DISABLE -I.


.SUFFIXES:

%_x.c: %.c
	perl cases.pl < $< > $@
%_x.h: %.h
	perl cases.pl < $< > $@

libusort.a: usort.o uhash.o ipsort.o
	ar -r $@ $^

timer:	timer.c usort.h usort.o ipsort.o uhash.o
	$(CC) $(CFLAGS) -o timer timer.c usort.o ipsort.o uhash.o

tester:	tester.c usort.h usort.o ipsort.o uhash.o
	$(CC) $(CFLAGS) -o tester tester.c usort.o ipsort.o uhash.o

usort.o: usort_x.c usort.h ipsort_x.h uhash_x.h utypes.h 
	$(CC) $(CFLAGS) -DCACHE_MB=$(CACHEMB) -o $@ -c usort_x.c

uhash.o: uhash_x.c uhash_x.h utypes.h 
	$(CC) $(CFLAGS) -o $@ -c uhash_x.c

ipsort.o: ipsort_x.c ipsort_x.h utypes.h 
	$(CC) $(CFLAGS) -o $@ -c ipsort_x.c

insert.o: insert.c
	$(CC) $(CFLAGS) -o $@ -c insert.c

usort.tgz: cases.pl tester.c timer.c usort.c uhash.c ipsort.c \
usort.h utypes.h uhash.h ipsort.h makefile cachemb.awk README
	tar -czf $@ $^

match: match.c singles.o
	$(CC) $(CFLAGS) -o match $^

insert: insert.c libusort.a
	$(CC) $(CFLAGS) -DTEST -o $@ insert.c -L. -lusort -lm

clean:
	rm -f tester timer match *.o *_x.c *_x.h libusort.a usort.tgz
