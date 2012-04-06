# include common variable definitions
include ../include.mk

SOFTXMT_SYS=../system

CFLAGS_COMMON= -fomit-frame-pointer -O1
#-Wall
CFLAGS+= $(CFLAGS_COMMON)
CFLAGS+= -I$(SOFTXMT_SYS)

CFLAGS+= -DGASNET_SEQ -DGASNET_CONDUIT_IBV

%.o: %.c
	$(CXX) $(CFLAGS)  $< -c -o $@

objects: global_memory.o global_array.o

clean:
	rm -f ./*.o
