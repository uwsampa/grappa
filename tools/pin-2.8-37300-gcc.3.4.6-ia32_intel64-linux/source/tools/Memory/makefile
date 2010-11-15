##
## PIN tools
##

##############################################################
#
# Here are some things you might want to configure
#
##############################################################

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

##############################################################
#
# include *.config files
#
##############################################################

ifeq ($(TARGET_COMPILER),gnu)
    include ../makefile.gnu.config
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT)
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

TOOL_ROOTS = icache dcache allcache dcache_xscale_config footprint memalloc memalloc2

all: tools

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

tools: $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TOOL_ROOTS:%=%.test)


## build rules

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

##
## In this test the tool does repeated mallocs in it's Fini function until it gets a NULL return value
## It tests that PIN's malloc supplied to the tool correctly returns NULL when out of memory
## A separate test is still needed to get PIN to internally exhaust memory and see that PIN
## outputs the "Out of memory" message to the pin logfile before exiting.

memalloc.test: $(OBJDIR)memalloc$(PINTOOL_SUFFIX) memalloc.tested memalloc.failed
	touch memalloc.out; rm memalloc.out
	$(PIN) -t $< -o memalloc.out -- $(TESTAPP) makefile $<.makefile.copy
	grep -q NULL memalloc.out
	rm memalloc.failed memalloc.out
	
memalloc2.test: $(OBJDIR)memalloc2$(PINTOOL_SUFFIX) memalloc2.tested memalloc2.failed
	touch memalloc2.out; rm memalloc2.out
	$(PIN) -t $< -o  memalloc2.out -- $(TESTAPP) makefile $<.makefile.copy
	grep -q OutOfMem memalloc2.out
	rm memalloc2.out memalloc2.failed

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed
