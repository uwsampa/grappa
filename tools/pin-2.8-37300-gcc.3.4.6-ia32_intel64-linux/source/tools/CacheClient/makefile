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
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
    ifeq ($(TARGET_OS),w)
        BIGBINARYFLAGS = -mno-cygwin
    else
	BIGBINARYFLAGS ?= 
    endif
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    BIGBINARYFLAGS ?= /nologo
    DBG?=
endif


#bb_test and orig_address are not run on cygwin due to inapility to print from Fini function because on
#cygwin the runtime is shared between the app and the tool
ifneq ($(TARGET_OS),w)
    SPECIALRUN = bb_test 
    NORMALRUN = cache_simulator watch_fragmentation trace_insertions enter_exit link_unlink event_trace insertDelete deleteTrace  orig_address
else
    ifeq ($(TARGET_COMPILER),gnu)
        SPECIALRUN = 
        NORMALRUN = cache_simulator watch_fragmentation trace_insertions enter_exit link_unlink event_trace insertDelete deleteTrace 
    else
        SPECIALRUN = bb_test
        NORMALRUN = cache_simulator watch_fragmentation trace_insertions enter_exit link_unlink event_trace insertDelete deleteTrace  orig_address
    endif
endif

# The br_test tests how pin handles big region.
NORMALRUN += br_test

ifeq ($(TARGET),ia32e)
    # The big_region_ia32e_test tests how pin handles big region. This is a problem
    # only in IA32E because it uses contingueus cache and it has hard
    # limit of 64K on the size of code cache block.

    TEST_NOTOOL += big_region_ia32e
endif

NORMALRUN += mem_usage cache_flusher cache_stats flush_leaks flush_at_if

TEST_NOTOOL = cache_block high_water 


TOOL_ROOTS = $(SPECIALRUN) $(NORMALRUN)

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

SANITY_TOOLS = 

THREADTESTAPP = ./$(OBJDIR)threadWait$(EXEEXT)
BIGTESTAPP = ./$(OBJDIR)bigBinary$(EXEEXT)
BIGREGIONAPP = ./$(OBJDIR)bigRegion$(EXEEXT)

#OS Dependent
ifeq ($(TARGET_OS),l)
SPECIALRUN += mtFlush
endif

CACHELIMIT = 262144
CACHEBLOCK = 131072
HIGHWATER = 75

ifeq ($(TARGET_OS),w)
CACHELIMIT_FOR_ACTION_PENDING = 10485760
else
CACHELIMIT_FOR_ACTION_PENDING = 2621440
endif


# Pin for the Intel(R) 64 architecture does not support variable size caches
#also  cache_doubler is not run on cygwin due to inapility to print from Fini function because on
#cygwin the runtime is shared between the app and the tool
ifneq ($(TARGET),ia32e)
    ifneq ($(TARGET_OS),w)
        TOOL_ROOTS += cache_doubler
    else
        ifneq ($(TARGET_COMPILER),gnu)
            TOOL_ROOTS += cache_doubler
        endif
    endif
endif

ifeq ($(TARGET),ia32e)
    TOOL_ROOTS += action_pending_cachefull
endif

THREAD_LIB=$(OBJDIR)threadlib.$(OBJEXT)
ifeq ($(TARGET_OS),w)
    OS_API=windows
else
    OS_API=unix
endif

ifneq ($(TARGET),ipf)
	TESTS_TO_RUN += flush_at_if_no_inline_bridge.test
endif

TEST_TOOLS = $(TEST_TOOLS_ROOTS:%=%.test)

all: tools 
tools: $(OBJDIR) $(TOOLS) 
test: $(OBJDIR) $(TOOL_ROOTS:%=%.test)  $(TEST_NOTOOL:%=%.test) $(TESTS_TO_RUN) 
tests-sanity: $(OBJDIR) $(SANITY_TOOLS:%=%.test)

$(OBJDIR):
	mkdir -p $(OBJDIR)


## special testing rules

action_pending_cachefull.test: $(OBJDIR)action_pending_cachefull$(PINTOOL_SUFFIX) action_pending_cachefull.tested action_pending_cachefull.failed $(OBJDIR)action_pending_cachefull_app
	$(PIN) -xyzzy -high_water_mark 25 -limit_code_cache $(CACHELIMIT_FOR_ACTION_PENDING) -t $<  -- $(OBJDIR)action_pending_cachefull_app
	rm action_pending_cachefull.failed

cache_flusher.test: $(OBJDIR)cache_flusher$(PINTOOL_SUFFIX) cache_flusher.tested cache_flusher.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -limit_code_cache $(CACHELIMIT) -cache_block_size 65536 -t $< -o $<.out -- $(BIGTESTAPP)
	grep -q SWOOSH $<.out
	grep -q eof $<.out
	rm cache_flusher.failed; rm $<.out

cache_doubler.test: $(OBJDIR)cache_doubler$(PINTOOL_SUFFIX) cache_doubler.tested cache_doubler.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -limit_code_cache $(CACHELIMIT) -cache_block_size 65536 -t $< -o $<.out -- $(BIGTESTAPP) 
	grep -q STRETCH $<.out
	grep -q eof $<.out
	rm cache_doubler.failed; rm $<.out

flushAndInsert.test: $(OBJDIR)flushAndInsert$(PINTOOL_SUFFIX) flushAndInsert.tested flushAndInsert.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -limit_code_cache $(CACHELIMIT) -cache_block_size 65536 -t $< -o $<.out -- $(BIGTESTAPP) 
	grep -q SWOOSH $<.out
	grep -q insertion $<.out
	rm flushAndInsert.failed; rm $<.out

cache_block.test: cache_block.tested cache_block.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -cache_block_size $(CACHEBLOCK) -- $(BIGTESTAPP)
	rm cache_block.failed

high_water.test: high_water.tested high_water.failed $(BIGTESTAPP)
	$(PIN) -xyzzy -high_water_mark $(HIGHWATER) -- $(BIGTESTAPP)
	rm high_water.failed

bb_test.test: $(OBJDIR)bb_test$(PINTOOL_SUFFIX) bb_test.tested bb_test.failed $(BIGTESTAPP)
	$(PIN) -t $< -b 10 -o $<.out -- $(BIGTESTAPP)
	grep -q "Final trace count" $<.out
	grep -q -v "Final trace count: 0" $<.out
	grep -q -v "Final cache size: 0" $<.out
	rm bb_test.failed; rm $<.out


mtFlush.test: $(OBJDIR)mtFlush$(PINTOOL_SUFFIX) mtFlush.tested mtFlush.failed $(THREADTESTAPP)
	$(PIN)  -t $< -- $(THREADTESTAPP) 
	grep -q SWOOSH mtFlush.out
	rm mtFlush.failed; rm mtFlush.out

flush_leaks.test: $(OBJDIR)flush_leaks$(PINTOOL_SUFFIX) flush_leaks.tested flush_leaks.failed ./$(OBJDIR)manyIndirects$(EXEEXT)
	$(PIN) -t $< -- ./$(OBJDIR)manyIndirects$(EXEEXT)
	rm flush_leaks.failed

orig_address.test:$(OBJDIR)orig_address$(PINTOOL_SUFFIX) orig_address.tested orig_address.failed $(BIGTESTAPP)
	$(PIN) -t $< -o $<.out -- $(BIGTESTAPP) 
	grep -q "Success" $<.out
	grep -q -v "Failure" $<.out
	grep -q "eof" $<.out
	rm orig_address.failed; rm $<.out

big_region_ia32e.test: %.test: $(OBJDIR)br_test$(PINTOOL_SUFFIX) %.tested %.failed $(BIGREGIONAPP)
	$(PIN) -xyzzy -inline_maxlen 100 -cache_block_size 65536 -region-max-inss 5000 -mesgon warning -logfile $*.log -t $< -o $*.out -- $(BIGREGIONAPP)
	grep -q "Attempting to insert" $*.log
	rm -f $*.failed $*.out $*.log
	
flush_at_if_no_inline_bridge.test: $(OBJDIR)flush_at_if${PINTOOL_SUFFIX} flush_at_if_no_inline_bridge.tested flush_at_if_no_inline_bridge.failed
	$(PIN) -xyzzy -inline_bridge 0 -t $(OBJDIR)flush_at_if$(PINTOOL_SUFFIX) -- $(TESTAPP) makefile flush_at_if_no_inline_bridge.makefile.copy
	$(PIN_CMP) makefile flush_at_if_no_inline_bridge.makefile.copy
	rm flush_at_if_no_inline_bridge.makefile.copy; rm $(@:.test=.failed)

br_test.test: %.test: $(OBJDIR)br_test$(PINTOOL_SUFFIX) %.tested %.failed $(BIGREGIONAPP)
	$(PIN) -xyzzy -inline_maxlen 1 -cache_block_size 65536 -region-max-inss 5000 -t $< -o $*.out -- $(BIGREGIONAPP)
	rm $*.failed $*.out
	
$(OBJDIR)threadlib.$(OBJEXT): ../threadlib/threadlib_$(OS_API).c ../threadlib/threadlib.h
	$(CC) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 

$(OBJDIR)threadWait: threadWait.c
	$(CC) $(APP_CXXFLAGS) $(DBG) -o $@ $< $(APP_PTHREAD)

$(OBJDIR)bigBinary$(EXEEXT): bigBinary.c
	$(CC) $(APP_CXXFLAGS) $(BIGBINARYFLAGS) ${OUTEXE}$@ $<

$(OBJDIR)action_pending_cachefull_app: action_pending_cachefull_app.c ../threadlib/threadlib.h $(THREAD_LIB)
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

$(OBJDIR)bigRegion$(EXEEXT): bigRegion.c
	$(CC) $(APP_CXXFLAGS) $(NO_OPTIMIZE) ${OUTEXE}$@ $<

$(OBJDIR)manyIndirects$(EXEEXT): manyIndirects.c
	$(CC) $(APP_CXXFLAGS) ${OUTEXE}$@ $<

## build rules

$(OBJDIR)%.o : %.cpp 
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o $(OBJDIR)utils.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(OBJDIR)utils.o $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed *.exp *.lib 

tidy:
	-rm -rf $(OBJDIR) *.tested *.failed

-include *.d
