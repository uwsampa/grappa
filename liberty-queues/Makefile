CC=gcc
LLVMCC=llvm-gcc
AR=ar

WARNINGS=-Wall \
	-Wextra \
	-Wconversion \
	-Wshadow \
	-Wcast-qual \
	-Wstrict-prototypes \
	-Winit-self \
	-Wmissing-include-dirs \
	-Wc++-compat \
	-Winline \
	-Werror

DEFINE=-D_BSD_SOURCE \
	-D_POSIX_C_SOURCE=199309L

CFLAGS=-std=c99 -g -O3 -pedantic -fPIC ${DEFINE} ${WARNINGS}

LDFLAGS=-lrt

SRCS=$(shell ls *.c)

.SECONDARY:

all : test_astream

test_astream : sw_queue.o

%.o : %.c
	${CC} ${CFLAGS} $< -c -o $@

%.s : %.c
	${CC} ${CFLAGS} $< -S -o $@

%.unit.o : %.c
	${CC} ${CFLAGS} -DUNIT_TEST $< -c -o $@

%.noinline.o : %.c
	${CC} ${CFLAGS} -DNO_INLINE $< -c -o $@

%.instr.o : %.c
	${CC} ${CFLAGS} -DINSTRUMENT $< -c -o $@

% : %.o
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o $@

%.a :
	${AR} rcs $@ $^

%.so :
	${CC} ${CFLAGS} ${LDFLAGS} -shared $^ -o $@

%.bc : %.c
	${LLVMCC} ${CFLAGS} -DNO_INLINE $< -c -o $@ -emit-llvm

clean:
	rm -f *~
	rm -f *.o
	rm -f *.d.*
	rm -f *.d
	rm -f *\#
	rm -f *.a
	rm -f *.s
	rm -f *.so
	rm -f *.bc
	rm -f a.out
	rm -f test_astream.noinline
	rm -f test_astream

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) -DUNIT_TEST $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o \1.unit.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SRCS:.c=.d)
