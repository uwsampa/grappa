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

SUF = $(PINTOOLS_SUFFIX)

##############################################################
#
# include *.config files
#
##############################################################

ifeq ($(TARGET_COMPILER),gnu)
    include ../makefile.gnu.config
    STATIC=-static
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    DBG?=
endif

ifeq ($(TARGET_OS),w)
    OS_API=windows
else
    OS_API=unix
endif

ifneq ($(ENABLE_VS), 1)
    VS_FLAG = -xyzzy -virtual_segments 1
else
    VS_FLAG =
endif
## Set up tool roots

# replay_change_main is no longer needed...
TOOL_ROOTS = 

# Windows only tests
ifeq ($(TARGET_OS),w)
    TOOL_ROOTS += win_replay_exception
    APPS       += win_exception
endif

# Linux only tests.
ifeq ($(TARGET_OS),l)
    # Not on ipf since the tests uses IARG_CONTEXT
    ifneq ($(TARGET),ipf)
        TOOL_ROOTS += record_imageload
    endif
endif

# Not on ipf since the tests uses IARG_CONTEXT
ifneq ($(TARGET),ipf)
    TOOL_ROOTS += replay_syscall
endif

# every tool should be tested
TEST_TOOLS = $(TOOL_ROOTS)
TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
APPS_BINARY_FILES = $(APPS:%=$(OBJDIR)%$(EXEEXT))

all: tools $(APPS_BINARY_FILES)

tools: $(OBJDIR) $(TOOLS)

## sanity
SANITY_TESTS = $(TEST_TOOLS:%=%.test)
TESTS_TO_RUN = $(SANITY_TESTS)

tests-sanity: $(OBJDIR) $(SANITY_TESTS)

test: $(OBJDIR) $(TESTS_TO_RUN)

$(OBJDIR):
	mkdir -p $(OBJDIR)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.output  *.out *.tested *.failed *.xml *.makefile.copy 
	-rm -rf pin.log pintool.log ctxtsave.txt memsave.txt core core.* *.core image* *.log
	-rm -rf obj-*

## Individual tests which are not validated by using cp and comparing the output...
#
# Test replaying the same exception (Windows only)
win_replay_exception.test: %.test : $(OBJDIR)win_replay_exception$(PINTOOL_SUFFIX) $(OBJDIR)win_exception %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)win_exception$(EXEEXT)
	rm win_replay_exception.failed win_replay_exception.out 

# Test replaying the same system call
replay_syscall.test: %.test : $(OBJDIR)replay_syscall$(PINTOOL_SUFFIX) %.tested %.failed
	$(PIN) -t $< -- $(TESTAPP) makefile $*.makefile.copy
	rm replay_syscall.failed replay_syscall.out 

# Test record and replay of image load/unload events
record_imageload.test: %.test : $(OBJDIR)record_imageload$(PINTOOL_SUFFIX) %.tested %.failed
        # Record the image load/unload information
	$(PIN) -t $< -- $(TESTAPP) makefile $*.makefile.copy
        # Replay it
	$(PIN) -t $< -r -- $(TESTAPP) makefile $*.makefile.copy
        # The output log should be the same at record and replay time
	diff record_imageload_rec.out record_imageload_play.out
	-rm record_imageload.failed record_imageload*.out imageload.log $*.makefile.copy

## build rules

$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)%.o : %.c
	$(CC) ${COPT} $(CFLAGS) $(PIN_CFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)%$(EXEEXT) : %.c
	$(CC) $(APPCFLAGS) ${OUTEXE}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)
