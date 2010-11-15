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
# Tools sets
#
##############################################################

TOOL_ROOTS = icount filter
CONTEXT_TOOLS = alarm_icount alarm_symbol control alarm_image_offset
ifeq ($(DETACH_SUPPORTED), yes)
    CONTEXT_TOOLS += control_detach
endif
SANITY_TOOLS = icount

ifneq ($(TARGET_OS),w)
	TOOL_ROOTS += follow_child
endif

TOOL_ROOTS += $(CONTEXT_TOOLS)

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))


##############################################################
#
# build rules
#
##############################################################

all: tools

tools: $(OBJDIR) $(TOOLS)
TESTS =  $(OBJDIR) $(TOOL_ROOTS:%=%.test)
ifeq ($(TARGET_OS),l)
   ifneq ($(TARGET), ipf)
      TESTS += marker_test int3_test
   endif

   # mt3_test has problems on old linux runtimes where thread stack 
   #is not aligned as the compiler assumes it should be aligned. Restricting
   #testing to 64b systems.
   ifeq ($(TARGET), ia32e)
      TESTS +=  mt3_test
   endif

endif
test: $(TESTS)
tests-sanity: $(OBJDIR) $(SANITY_TOOLS:%=%.test)

control_detach.test : $(OBJDIR)control_detach$(PINTOOL_SUFFIX) control_detach.tested control_detach.failed 
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -skip 1000  -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	rm $<.makefile.copy; rm control_detach.failed

## build rules
$(OBJDIR):
	mkdir -p $(OBJDIR)

# gcc linux test only
$(OBJDIR)itext-marker-test:  itext-marker-test.c
	$(CC) $(APP_CXXFLAGS) -o $@ $^

marker_test: $(OBJDIR)itext-marker-test  $(OBJDIR)control$(PINTOOL_SUFFIX)
	touch marker_test.failed
	$(PIN) -t $(OBJDIR)control$(PINTOOL_SUFFIX)  -controller-start-itext 0f1ff3 -controller-stop-itext 0f1ff4 -- $(OBJDIR)itext-marker-test > marker_test.out 2>&1
	grep -E '0x.*Start' marker_test.out
	grep -E '0x.*Stop' marker_test.out
	rm marker_test.failed

# gcc linux test only
$(OBJDIR)int3-test:  int3-test.c
	$(CC) $(APP_CXXFLAGS) -o $@ $^

int3_test: $(OBJDIR)int3-test  $(OBJDIR)control$(PINTOOL_SUFFIX)
	touch int3_test.failed
	$(PIN) -t $(OBJDIR)control$(PINTOOL_SUFFIX)  -controller-start-int3 2 -controller-stop-int3 3 -- $(OBJDIR)int3-test > int3_test.out 2>&1
	grep -E '0x.*Start' int3_test.out
	grep -E '0x.*Stop' int3_test.out
	rm int3_test.failed


# gcc linux test only
$(OBJDIR)mt3-test:  test-mt3.cpp
	$(CXX) $(APP_CXXFLAGS) -o $@ $^ $(APP_PTHREAD) -msse2

mt3_test: $(OBJDIR)mt3-test  $(OBJDIR)control$(PINTOOL_SUFFIX)
	touch mt3_test.failed
	$(PIN) -t $(OBJDIR)control$(PINTOOL_SUFFIX)  -controller-start-ssc-mark 44332211 -controller-stop-ssc-mark 55332211 -- $(OBJDIR)mt3-test > mt3_test.out 2>&1
	grep -E '0x.*Start' mt3_test.out
	grep -E '0x.*Stop' mt3_test.out
	rm mt3_test.failed mt3_test.out

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed

-include *.d
