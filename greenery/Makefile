GENFLAGS= -O2 -Wall -Wextra -g -fopenmp
CFLAGS= $(GENFLAGS) -std=c99
LDFLAGS= $(GENFLAGS)  -lnuma -lhugetlbfs
ASFLAGS= $(GENFLAGS)

.PHONY: clean all

COMPONENTS= stack coro thread
PROGRAMS=corotest threadtest carlson
LIBS=libgreenery.a

all: $(PROGRAMS) $(LIBS) $(DIS)
	objdump -S carlson > carlson.dis

libgreenery.a: $(patsubst %,libgreenery.a(%.o),$(COMPONENTS))

corotest: $(LIBS) corotest.c

threadtest: $(LIBS) threadtest.c

carlson: $(LIBS) carlson.c

clean:
	rm -rf $(PROGRAMS) $(LIBS)
