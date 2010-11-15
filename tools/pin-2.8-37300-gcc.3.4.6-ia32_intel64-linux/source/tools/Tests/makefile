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

GDB = /usr/bin/gdb

##############################################################
#
# include *.config files
#
##############################################################

ifeq ($(TARGET_COMPILER),gnu)
    include ../makefile.gnu.config
    OPT?=-O2 -fomit-frame-pointer
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    DBG?=
endif

#uninit-new
TOOL_ROOTS = removeinstrumentation segv probe after \
        hello icount1 icount2 icount3 icount4 icount5 icountcheck IARGList \
        sequence sequencecheck reg brtaken verify bblnull \
        imageLoad imageLoadBackward callargs traceusage \
        malloc new \
        sec imageAddress bblCache traceOriginal finiRemove \
        tracecount smc_check multipleinstrumentation win_cvapp_test_tool inscount2_mt_debuginfo
ifeq ($(DETACH_SUPPORTED), yes)
    TOOL_ROOTS += detach
endif

ifneq ($(TARGET_OS),w)
    TOOL_ROOTS += mtest cflowcheck 
endif
ifeq ($(TARGET_OS),w)
  ifeq ($(TARGET_COMPILER),ms)
    TOOL_ROOTS += cflowcheck
  endif
endif

TEST_TOOLS_ROOTS += snprintf_non_inlined_bridge

ifeq ($(TARGET_OS),l)
  ifeq ($(SOTOOL),1)
    TEST_TOOLS_ROOTS += toolwithoutso toolpathwithoutso
  endif
endif

# leave out probe because it causes an early exit
TEST_TOOLS_ROOTS = after hello icount1 icount2 icount3 icount4 icount5 \
    icountcheck sequence sequencecheck reg brtaken verify bblnull sec \
    cflowcheck callargs malloc new removeinstrumentation \
    IARGList imageLoadBackward imageAddress bblCache traceOriginal finiRemove
ifeq ($(DETACH_SUPPORTED), yes)
    TEST_TOOLS_ROOTS += detach
endif


ifeq ($(TARGET),ia32e)
    TOOL_ROOTS += snprintf
    TEST_TOOLS_ROOTS += snprintf verify-nocmov
    ifeq ($(TARGET_OS),l)
        TEST_TOOLS_ROOTS += checkaddress64
	TOOL_ROOTS += checkaddress ea_verifier	
    endif
    ifeq ($(TARGET_OS),w)
	    TOOL_ROOTS += ea_verifier
    endif
endif

ifeq ($(TARGET),ia32)
    TOOL_ROOTS += snprintf alah emu checkaddress operand syntax 
    TEST_TOOLS_ROOTS += snprintf alah emu emu-bridge checkaddress operand verify-nocmov syntax 
    ifeq ($(TARGET_OS),l)
        TEST_TOOLS_ROOTS += verify-ia32 verify-ia32-nocmov limitedTraces thread \
                            symOutOfBound util prefix exitProcess
        TOOL_ROOTS += ea_verifier limitedTraces thread symOutOfBound util prefix exitProcess
    endif
    ifeq ($(TARGET_OS),w)
        TEST_TOOLS_ROOTS += win-emu-bridge 
        TOOL_ROOTS += set_df_flag_tool clear_df_flag_tool set_status_flags_tool ea_verifier
    endif   
endif

ifeq ($(TARGET),ipf)
    TEST_TOOLS_ROOTS += verify-ipf branch-ipf 
    TOOL_ROOTS += operand-ipf
endif

ifeq ($(TARGET_OS),w)
  ifeq ($(TARGET_COMPILER),gnu)
    CPPIN_FLAGS = -mno-cygwin
  else
    CPPIN_FLAGS = /nologo /EHsc /MD
  endif
  TEST_APPS  = $(OBJDIR)cp-pin
else
  ifeq (${HOST_ARCH},ia32e)
	ifeq (${TARGET},ia32)
	  CPPIN_FLAGS = -m32
	  TEST_APPS = $(OBJDIR)cp-pin
	endif
  endif
endif

ifndef TEST_APPS 
  TEST_APPS = 
endif

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
TEST_TOOLS = $(TEST_TOOLS_ROOTS:%=%.test)

all:  tools  
tools: $(OBJDIR)
	-$(MAKE) run_tools
run_tools: $(TOOLS) $(OBJDIR)foobar
test-apps: $(OBJDIR)
	-$(MAKE) run_test-apps
run_test-apps: $(TEST_APPS)
tests: $(OBJDIR)
	-$(MAKE) run_tests
run_tests: $(TEST_TOOLS)

## sanity

SANITY_TOOLS_ROOTS =  icount2 verify after brtaken sequence
ifneq ($(TARGET_OS),w)
   #FIXME: image api
   SANITY_TOOLS_ROOTS += callargs
endif

SANITY_TOOLS = $(SANITY_TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
SANITY_TESTS = $(SANITY_TOOL_ROOTS:%=%.test)

tools-sanity: $(OBJDIR) $(SANITY_TOOLS)
tests-sanity: $(OBJDIR) $(SANITY_TESTS)
test:         $(OBJDIR) $(TEST_TOOLS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

callargs.test: $(OBJDIR)callargs${PINTOOL_SUFFIX} callargs.tested callargs.failed $(OBJDIR)foobar
	$(PIN) -t $< -- ./$(OBJDIR)foobar
	rm callargs.failed

toolwithoutso.test: $(OBJDIR)callargs${PINTOOL_SUFFIX} toolwithoutso.tested toolwithoutso.failed $(OBJDIR)foobar
	$(PIN) -t $(subst ${PINTOOL_SUFFIX},,$<) -- ./$(OBJDIR)foobar
	rm toolwithoutso.failed

toolpathwithoutso.test: $(OBJDIR)callargs${PINTOOL_SUFFIX} toolpathwithoutso.tested toolpathwithoutso.failed $(OBJDIR)foobar
	$(PIN) -t ../Tests/$(subst ${PINTOOL_SUFFIX},,$<) -- ./$(OBJDIR)foobar
	rm toolpathwithoutso.failed

verify-ia32.test: $(OBJDIR)verify${PINTOOL_SUFFIX} verify-ia32.tested verify-ia32.failed $(OBJDIR)loop
	$(PIN) -t $< -- ./$(OBJDIR)loop
	rm verify-ia32.failed
	
snprintf_non_inlined_bridge.test: $(OBJDIR)snprintf${PINTOOL_SUFFIX} snprintf_non_inlined_bridge.tested snprintf_non_inlined_bridge.failed
	$(PIN) -xyzzy -inline_bridge 0 -t $(OBJDIR)snprintf$(PINTOOL_SUFFIX) -- $(TESTAPP) makefile snprintf_non_inlined_bridge.makefile.copy
	$(PIN_CMP) makefile snprintf_non_inlined_bridge.makefile.copy
	rm snprintf_non_inlined_bridge.makefile.copy; rm $(@:.test=.failed)

verify-ia32-nocmov.test: $(OBJDIR)verify${PINTOOL_SUFFIX} verify-ia32-nocmov.tested verify-ia32-nocmov.failed $(OBJDIR)loop
	$(PIN) -xyzzy -nocmov 1 -t $< -- ./$(OBJDIR)loop
	rm verify-ia32-nocmov.failed

verify-nocmov.test : $(OBJDIR)verify$(PINTOOL_SUFFIX) verify-nocmov.tested verify-nocmov.failed $(TESTAPP)
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -nocmov 1 -t $(OBJDIR)verify$(PINTOOL_SUFFIX) -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	rm $<.makefile.copy; rm $(@:.test=.failed)

verify-ipf.test: $(OBJDIR)verify${PINTOOL_SUFFIX} verify-ipf.tested verify-ipf.failed $(OBJDIR)ipf_branch
	$(PIN) -t $< -- ./$(OBJDIR)ipf_branch
	rm verify-ipf.failed

branch-ipf.test: branch-ipf.tested branch-ipf.failed $(OBJDIR)ipf_branch
	$(PIN) -- ./$(OBJDIR)ipf_branch
	rm branch-ipf.failed

check_env.test: $(OBJDIR)hello${PINTOOL_SUFFIX} check_env.tested check_env.failed
	./check_env.sh > check_env.before
	$(PIN) -t $< -- ./check_env.sh > check_env.tool.after
	$(PIN) -- ./check_env.sh > check_env.notool.after
	$(PIN_CMP) check_env.before check_env.tool.after
	$(PIN_CMP) check_env.before check_env.notool.after
	rm check_env.before check_env.tool.after check_env.notool.after check_env.failed

emu-bridge.test: $(OBJDIR)emu${PINTOOL_SUFFIX} emu-bridge.tested emu-bridge.failed
	touch emu-bridge.makefile.copy; rm emu-bridge.makefile.copy
	$(PIN) -xyzzy -inline 0 -t $< -- $(TESTAPP) makefile emu-bridge.makefile.copy
	$(PIN_CMP) makefile emu-bridge.makefile.copy
	rm emu-bridge.makefile.copy; rm emu-bridge.failed

win-emu-bridge.test: $(OBJDIR)emu${PINTOOL_SUFFIX} win-emu-bridge.tested win-emu-bridge.failed $(OBJDIR)lasterror
	$(PIN) -xyzzy -inline 0 -t $< -- $(OBJDIR)lasterror
	rm win-emu-bridge.failed	

checkaddress64.test: $(OBJDIR)checkaddress${PINTOOL_SUFFIX} checkaddress64.tested checkaddress64.failed $(OBJDIR)checkaddressapp
	$(PIN) -t $< -- ./$(OBJDIR)checkaddressapp
	rm checkaddress64.failed

thread.test: $(OBJDIR)thread${PINTOOL_SUFFIX} thread.tested thread.failed $(OBJDIR)threadApp
	$(PIN) -t $< -- ./$(OBJDIR)threadApp
	rm thread.failed

util.test: $(OBJDIR)util${PINTOOL_SUFFIX} util.tested util.failed $(TESTAPP)
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	rm util.failed

prefix.test: $(OBJDIR)prefix${PINTOOL_SUFFIX} prefix.tested prefix.failed $(OBJDIR)prefixApp
	$(PIN) -t $< -- $(OBJDIR)prefixApp
	rm prefix.failed

exitProcess.test: $(OBJDIR)exitProcess${PINTOOL_SUFFIX} exitProcess.tested exitProcess.failed $(OBJDIR)exitProcessApp
	$(PIN) -t $< -- $(OBJDIR)exitProcessApp
	rm exitProcess.failed

$(OBJDIR)checkaddressapp: checkaddressapp.s
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)checkaddressapp $<

$(OBJDIR)prefixApp: prefixApp.s
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)prefixApp $< -g

$(OBJDIR)foobar: foobar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ $<

$(OBJDIR)cp-pin: cp-pin.cpp
	${CXX} $(CPPIN_FLAGS) $(APP_CXXFLAGS) ${OUTEXE}$@ $<

$(OBJDIR)ipf_branch: ipf_branch.s
	$(CC) $(APP_CXXFLAGS) -static -o $@ ipf_branch.s

$(OBJDIR)loop: loop.s
	$(CC) $(APP_CXXFLAGS) -static -o $@ loop.s

$(OBJDIR)lasterror: lasterror.cpp
	$(CXX) $(APP_CXXFLAGS) ${OUTEXE}$@ $< 

$(OBJDIR)threadApp: threadApp.cpp
	$(CXX) $(APP_CXXFLAGS) -o $(OBJDIR)threadApp $< $(APP_PTHREAD)

$(OBJDIR)exitProcessApp: exitProcessApp.c
	$(CXX) $(APP_CXXFLAGS) ${OUTEXE}$@ $<

## build rules

$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(TOOLS): $(PIN_LIBNAMES)


$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed  $(TEST_APPS) *.manifest  pin.log 

-include *.d
