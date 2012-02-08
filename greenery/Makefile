CC=gcc


#GENFLAGS= -O1 -Wall -Wextra -g
GENFLAGS= -O3 -Wall -Wextra


CFLAGS= $(GENFLAGS) -std=gnu99
LDFLAGS= $(GENFLAGS)
ASFLAGS= $(GENFLAGS)

.PHONY: clean all

COMPONENTS= stack coro thread
PROGRAMS=corotest threadtest
LIBS=libgreenery.a

all: $(PROGRAMS) $(LIBS)

libgreenery.a: $(patsubst %,libgreenery.a(%.o),$(COMPONENTS))

corotest: $(LIBS) corotest.c

threadtest: $(LIBS) threadtest.c

clean:
	rm -rf $(PROGRAMS) $(LIBS)
