CFLAGS = -O2 -std=c99 -fomit-frame-pointer -Wall -Wextra
LDFLAGS = -O2 -fomit-frame-pointer -Wall -Wextra
ASFLAGS= -O2 -Wall -Wextra

.PHONY: clean all

all: libcoro.a corotest

libcoro.a: libcoro.a(stack.o) libcoro.a(coro.o)

corotest: -lcoro corotest.o


clean:
	rm -rf *.o libcoro.a corotest
