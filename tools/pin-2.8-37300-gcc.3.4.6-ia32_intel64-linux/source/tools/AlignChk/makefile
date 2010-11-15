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

ifeq ($(TARGET_COMPILER),ms)
   EXTRA_LIBS =
else
   EXTRA_LIBS =
endif

IA32_TEST_TOOLS = alignchk

ifeq ($(TARGET),ia32)
    TOOL_ROOTS += $(IA32_TEST_TOOLS)
    APPS = misaligned
endif
ifeq ($(TARGET),ia32e)
    TOOL_ROOTS += $(IA32_TEST_TOOLS)
    APPS = misaligned
endif


all: tools $(APPS:%=$(OBJDIR)%)

SANITY_TOOLS = 
# every tool should be tested
TEST_TOOLS_ROOTS = $(TOOL_ROOTS)

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

TEST_TOOLS = $(TEST_TOOLS_ROOTS:%=%$(PINTOOL_SUFFIX))

tools: $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TEST_TOOLS:%=%.test)
tests-sanity: $(OBJDIR) $(SANITY_TOOLS:%=%.test)

## build rules

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)

ifeq ($(TARGET),ia32e)
    ifeq ($(TARGET_COMPILER),ms)
        SSE_ASM_OBJ     = $(OBJDIR)movdqa_test_ia32e.obj
        LINK_OPTION     = /link
    endif
endif

ifeq ($(TARGET),ia32)
    ifeq ($(TARGET_COMPILER),ms)
        SSE_ASM_OBJ     = $(OBJDIR)movdqa_test_ia32.obj
        LINK_OPTION     = /link
    endif
endif

$(OBJDIR)misaligned: misaligned.c $(SSE_ASM_OBJ)
	${CC} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTEXE}$@ $< $(LINK_OPTION) $(SSE_ASM_OBJ)

$(OBJDIR)movdqa_test_ia32.obj: movdqa_test_ia32.asm
	ml /nologo $(OUTOPT) $@ /c $< 

$(OBJDIR)movdqa_test_ia32e.obj: movdqa_test_ia32e.asm
	ml64 /nologo $(OUTOPT) $@ /c $< 

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed *.exp *.lib 

alignchk$(PINTOOL_SUFFIX).test : $(OBJDIR)alignchk$(PINTOOL_SUFFIX) $(OBJDIR)misaligned alignchk.tested alignchk.failed 
	-rm -f alignchk.out
	@echo This is supposed to fail and report a problem about an unaligned MOVDQA
	-$(PIN) -t $(OBJDIR)alignchk$(PINTOOL_SUFFIX)  -- ./$(OBJDIR)misaligned  >  alignchk.out 2>&1
	grep "Misaligned MOVDQA at instruction" alignchk.out
	rm alignchk.failed  alignchk.out

-include $(OBJDIR)*.d
