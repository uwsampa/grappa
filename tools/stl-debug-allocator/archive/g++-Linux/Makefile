#
# $Id: Makefile,v 1.1 2001/10/21 22:09:34 cristiv Exp cristiv $
#

# Uncomment to build with multithread support
#MULTITHREAD=-D_MT

# Uncomment to intercept free() and mprotect()
DEFS+=-DUSE_WRAP

# Uncomment to use mmap() instead of pvalloc()
#DEFS+=-DUSE_MMAP

DEFS+=-D_DEBUG

export CXXFLAGS+=-g -fno-inline -DUNIX ${DEFS} ${MULTITHREAD}

ifeq ($(findstring -DUSE_WRAP, ${DEFS}), -DUSE_WRAP)
export LDFLAGS+=-Wl,--wrap,free,--wrap,mprotect
endif

ifeq (${MULTITHREAD}, -D_MT)
export LIBS+=-lpthread
endif

all: depend test-allocator test-threads

test-allocator:
	make -f test_allocator.make


ifeq (${MULTITHREAD}, -D_MT)
test-threads:
	make -f test_threads.make
else
test-threads:
endif

depend:
	g++ -D_MT -MM ../Source/*.cpp | sed -e 's/[^ ]*\.cpp//g' > depend.make


clean:
	make clean -e -f test_allocator.make
	make clean -f test_threads.make
	rm -rf depend.make
	
