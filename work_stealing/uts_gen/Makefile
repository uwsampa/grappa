
# ------------------------------------- #
# Set Random Number Generator sources:
# ------------------------------------- #

# Set the default RNG
ifndef RNG
RNG=BRG
endif

ifeq ($(RNG), Devine) 
RNG_SRC = rng/devine_sha1.c
RNG_INCL= rng/devine_sha1.h
RNG_DEF = -DDEVINE_RNG
endif
ifeq ($(RNG), BRG)
RNG_SRC = rng/brg_sha1.c
RNG_INCL= rng/brg_sha1.h
RNG_DEF = -DBRG_RNG
endif
ifeq ($(RNG), ALFG)
RNG_SRC = rng/alfg.c
RNG_INCL= rng/alfg.h
RNG_DEF = -DUTS_ALFG
endif
#-----------------------------------------#

THREAD_LIB=../../greenery

CC=gcc
CXX=g++
CFLAGS_COMMON= -Wno-write-strings -O3 -g -fopenmp
CFLAGS= $(CFLAGS_COMMON) -std=gnu99 -I./rng -I$(THREAD_LIB)
LIB=-lm -L$(THREAD_LIB) -lgreenery

HEADERS=\
	uts.h \
	StealQueue.h


T1=-t 1 -a 3 -d 10 -b 4 -r 19 
T1L=-t 1 -a 3 -d 13 -b 4 -r 29

TreeGen: TreeGen.c uts.c StealQueue.c $(RNG_SRC)
	$(CC) $(CFLAGS) $(RNG_DEF) -o $@ $+ $(LIB)

testLocal: testLocal.cpp StealQueue.c
	$(CXX) $(CFLAGS) -o $@ $+

run: TreeGen
	./TreeGen $(T1)

clean:
	rm -f ./*.o ./TreeGen
