# Makefile for CacheFilter
# Mark Charney
#########################################################################

##############################################################
#
# Here are some things you might want to configure
#
##############################################################

#TARGET_COMPILER?=ms
TARGET_COMPILER?=gnu

##############################################################
#
# include *.config files
#
##############################################################

ifeq ($(TARGET_COMPILER),gnu)
    include ../makefile.gnu.config
    LINKER?=${CXX}
    CXXFLAGS ?= -I$(PIN_HOME)/InstLib  -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    DBG?=
endif

##############################################################
#
# build rules
#
##############################################################

EXTRA_LIBS =

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<


TOOLS = cachefilt4
all: $(TOOLS)

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${LINKER} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)



cachefilt3: cachefilt3.o log-util.o

cachefilt3.o: cachefilt3.cpp Makefile

cachefilt4: cachefilt4.o log-util.o

cachefilt4.o: cachefilt4.cpp Makefile \
			caches.H my-types.H

clean:
	-rm -f *.o $(TOOLS) *.out *.tested *.failed *.d

-include *.d
