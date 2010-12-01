GENFLAGS= -O2 -Wall -Wextra -g
CFLAGS= $(GENFLAGS) -std=c99
LDFLAGS= $(GENFLAGS)
ASFLAGS= $(GENFLAGS)

.PHONY: clean all

COMPONENTS= stack coro thread
PROGRAMS=corotest threadtest carlson
LIBS=libgreenery.a -lnuma

all: $(PROGRAMS) $(LIBS)

libgreenery.a: $(patsubst %,libgreenery.a(%.o),$(COMPONENTS))

corotest: $(LIBS) corotest.c

threadtest: $(LIBS) threadtest.c

carlson: $(LIBS) carlson.c

clean:
	rm -rf $(PROGRAMS) $(LIBS)
