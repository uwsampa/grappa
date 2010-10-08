GENFLAGS= -O2 -Wall -Wextra
CFLAGS= $(GENFLAGS) -std=c99
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
