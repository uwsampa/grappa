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
    CXXFLAGS ?= -I$(PIN_HOME)/InstLib -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
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

TOOL_ROOTS = memtrace memtrace_simple
APPS = thread

ifneq ($(TARGET),ipf)
TOOL_ROOTS += membuffer membuffer_simple 
endif

ifeq ($(TARGET_OS),w)
TOOL_ROOTS += membuffer_threadpool memtrace_threadpool 
endif

all: tools $(APPS:%=$(OBJDIR)%)
SANITY_TOOLS = 

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
TESTS = $(TOOL_ROOTS:%=%.test)

TESTS += memtracemt.test memtrace_simple_mt.test 

ifeq ($(TARGET_OS),w)
	TESTS += membuffer_threadpool_mt.test memtrace_threadpool_mt.test 
endif 

ifeq ($(TARGET_OS),l)
	TESTS += memtracemt.test 
ifneq ($(TARGET),ipf)
	TESTS += membuffermt.test membuffer_simple_mt.test
endif
endif

ifeq ($(TARGET_OS),w)
    OS_API=windows
else
    OS_API=unix
endif

THREAD_LIB=$(OBJDIR)threadlib.$(OBJEXT)

tools: $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TESTS)
tests-sanity: $(OBJDIR) $(SANITY_TOOLS:%=%.test)

## build rules
	

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG) 

$(OBJDIR)threadlib.$(OBJEXT): ../threadlib/threadlib_$(OS_API).c ../threadlib/threadlib.h
	$(CC) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 
	
memtracemt.test : $(OBJDIR)memtrace$(PINTOOL_SUFFIX) memtracemt.tested memtracemt.failed $(OBJDIR)thread
	$(PIN) -t $< -emit 0 -- $(OBJDIR)thread
	rm memtracemt.failed

membuffermt.test : $(OBJDIR)membuffer$(PINTOOL_SUFFIX) membuffermt.tested membuffermt.failed $(OBJDIR)thread
	$(PIN) -t $< -emit 0 -- $(OBJDIR)thread
	rm membuffermt.failed
	
membuffer_simple_mt.test : $(OBJDIR)membuffer_simple$(PINTOOL_SUFFIX) membuffer_simple_mt.tested membuffer_simple_mt.failed $(OBJDIR)thread
	$(PIN) -t $<  -- $(OBJDIR)thread
	rm membuffer_simple_mt.failed
	
memtrace_simple_mt.test : $(OBJDIR)memtrace_simple$(PINTOOL_SUFFIX) memtrace_simple_mt.tested memtrace_simple_mt.failed $(OBJDIR)thread
	$(PIN) -t $<  -- $(OBJDIR)thread
	rm memtrace_simple_mt.failed
	
membuffer_threadpool_mt.test : $(OBJDIR)membuffer_threadpool$(PINTOOL_SUFFIX) membuffer_threadpool_mt.tested membuffer_threadpool_mt.failed $(OBJDIR)thread
	$(PIN) -t $<  -- $(OBJDIR)thread
	rm membuffer_threadpool_mt.failed
	
memtrace_threadpool_mt.test : $(OBJDIR)memtrace_threadpool$(PINTOOL_SUFFIX) memtrace_threadpool_mt.tested memtrace_threadpool_mt.failed $(OBJDIR)thread
	$(PIN) -t $<  -- $(OBJDIR)thread
	rm memtrace_threadpool_mt.failed


## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.out.* *.tested *.failed


	
$(OBJDIR)thread: thread.c  ../threadlib/threadlib.h $(THREAD_LIB)
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)thread thread.c  $(THREAD_LIB) $(APP_PTHREAD)

%.test : $(OBJDIR)%$(PINTOOL_SUFFIX) %.tested %.failed
	touch $*.makefile.copy; rm $*.makefile.copy
	$(PIN) -t $<  -- $(TESTAPP) makefile $*.makefile.copy
	$(PIN_CMP) makefile $*.makefile.copy
	rm $*.makefile.copy; rm $*.failed

-include *.d
