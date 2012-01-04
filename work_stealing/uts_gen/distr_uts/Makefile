CXX=g++

# ------------------------------------- #
# Set Random Number Generator sources:
# ------------------------------------- #

# Set the default RNG
RNG_CONTDIR=..

ifndef RNG
RNG=BRG
endif

ifeq ($(RNG), Devine) 
RNG_SRC = $(RNG_CONTDIR)/rng/devine_sha1.c
RNG_INCL= $(RNG_CONTDIR)/rng/devine_sha1.h
RNG_DEF = -DDEVINE_RNG
endif
ifeq ($(RNG), BRG)
RNG_SRC = $(RNG_CONTDIR)/rng/brg_sha1.c
RNG_INCL=  $(RNG_CONTDIR)/rng/brg_sha1.h
RNG_DEF = -DBRG_RNG
endif
ifeq ($(RNG), ALFG)
RNG_SRC = $(RNG_CONTDIR)/rng/alfg.c
RNG_INCL= $(RNG_CONTDIR)/rng/alfg.h
RNG_DEF = -DUTS_ALFG
endif
#-----------------------------------------#

CFLAGS= -Wno-write-strings -O0 -g -fopenmp
CFLAGS+= -I../rng
TH_CFLAGS=-I../../../greenery 
TH_LIB=-L../../../greenery -lgreenery
LIB=-lm

SOURCES=\
	balloc.c 

TreeGen: TreeGen.c uts.c StealQueue.c $(RNG_SRC) $(SOURCES)
	$(CXX) $(CFLAGS) $(RNG_DEF) -o $@ $+ $(LIB)

clean:
	rm -f ./TreeGen ./*.o
