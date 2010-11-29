BLDTYPE=pin
PIN_HOME=../pin
PIN_KIT=../pin


PROG = CacheTestDriver
PINTOOL = MultiCacheSim_PinDriver.so

SRCS = MSI_SMPCache.cpp MESI_SMPCache.cpp SMPCache.cpp MultiCacheSim.cpp CacheCore.cpp Snippets.cpp nanassert.cpp

ifeq ($(BLDTYPE),pin)
include $(PIN_HOME)/source/tools/makefile.gnu.config
CXXFLAGS += -DPIN -Wno-deprecated
SRCS += MultiCacheSim_PinDriver.cpp
TARG = $(PINTOOL)
else
CXXFLAGS += -UPIN
LDFLAGS = -lpthread
SRCS += CacheTestDriver.cpp
TARG = $(PROG)
endif


CXXFLAGS += -I. -g -O0

OBJS = $(SRCS:%.cpp=%.o)


all: $(TARG)

test: $(PROG)
	./CacheTestDriver

## build rules
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<

$(PROG): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $+ $(DBG)

$(PINTOOL): $(OBJS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $+ $(PIN_LIBS) $(DBG)

## cleaning
clean:
	-rm -f *.o $(PROG) $(PINTOOL)

-include *.d
