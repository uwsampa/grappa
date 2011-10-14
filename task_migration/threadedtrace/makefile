PIN_PATH = ../pin-2.10

# check some things for safety
CFLAGS += -Wall -Werror -Wno-unknown-pragmas  -O3 -fomit-frame-pointer -DBIGARRAY_MULTIPLIER=1 -DUSING_XED  -fno-strict-aliasing

# includes
CFLAGS += -I$(PIN_PATH)/source/include -I$(PIN_PATH)/source/include/gen -I$(PIN_PATH)/extras/xed2-ia32/include -I$(PIN_PATH)/extras/components/include

#defines (for linux x86 32-bit)
CFLAGS += -DTARGET_IA32 -DHOST_IA32 -DTARGET_LINUX

# flags for making shared library for pin
SO_FLAGS += -Wl,--hash-style=sysv -shared -Wl,-Bsymbolic -Wl,--version-script=$(PIN_PATH)/source/include/pintool.ver

# linking
LIBS = 
LIBS += -L$(PIN_PATH)/extras/xed2-ia32/lib -L$(PIN_PATH)/ia32/lib -L$(PIN_PATH)/ia32/lib-ext -lpin -lxed -ldwarf -lelf -ldl

all: threadedtrace

threadedtrace: threadedtrace.o
	g++ $(SO_FLAGS) -o threadedtrace.so $^ $(LIBS)

threadedtrace.o: threadedtrace.cpp
	g++ -c $(CFLAGS) -o threadedtrace.o threadedtrace.cpp


