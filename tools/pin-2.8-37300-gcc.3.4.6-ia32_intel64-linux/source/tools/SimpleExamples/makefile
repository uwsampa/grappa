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
    CXXFLAGS ?= -I$(PIN_HOME)/InstLib -fomit-frame-pointer -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
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

TOOL_ROOTS = dcache edgcnt pinatrace trace icount inscount2_mt opcodemix malloctrace calltrace \
             jumpmix toprtn catmix regmix ilenmix coco

ifneq ($(TARGET),ia32e)
    TOOL_ROOTS += topopcode 
else
    ifneq ($(TARGET_COMPILER),ms)
        TOOL_ROOTS += topopcode
    endif
endif

ifeq (${TARGET_OS},l)
    TOOL_ROOTS += fence
endif

ifeq (${TARGET_OS},m)
    TOOL_ROOTS += fence
endif

ifeq ($(TARGET),ia32)
    TOOL_ROOTS += emuload extmix xed-print xed-use ldstmix 
    ifeq (${TARGET_OS},l)
        TOOL_ROOTS +=  xed-cache	
    endif
endif

ifeq ($(TARGET),ia32e)
    TOOL_ROOTS += ldstmix extmix xed-use xed-print inscount2_vregs
    ifeq (${TARGET_OS},l)
        TOOL_ROOTS +=  xed-cache	
    endif
endif

SANITY_TOOLS = opcodemix$(PINTOOL_SUFFIX) 
# leave out fence, see comment at top of fence.cpp

TEST_TOOLS_ROOTS = dcache edgcnt pinatrace trace icount opcodemix  malloctrace calltrace \
                   jumpmix toprtn catmix regmix inscount2_mt coco
ifeq ($(TARGET),ia32)
    TEST_TOOLS_ROOTS +=  emuload extmix xed-use xed-print ldstmix 
    ifeq (${TARGET_OS},l)
        TEST_TOOL_ROOTS +=  xed-cache	
    endif
endif
ifeq ($(TARGET),ia32e)
    TEST_TOOLS_ROOTS +=  extmix xed-use xed-print ldstmix
    ifeq (${TARGET_OS},l)
        TEST_TOOL_ROOTS +=  xed-cache	
    endif
endif
ifneq ($(TARGET),ia32e)
    TEST_TOOLS_ROOTS += topopcode
else
    ifneq ($(TARGET_COMPILER),ms)
        TEST_TOOLS_ROOTS += topopcode
    endif
endif

TEST_TOOLS = $(TEST_TOOLS_ROOTS:%=%$(PINTOOL_SUFFIX))

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))



all: tools
tools: $(OBJDIR) $(TOOLS)
test:  $(OBJDIR) $(TEST_TOOLS:%=%.test)
tests-sanity: $(SANITY_TOOLS:%=%.test)

## build rules

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	${CXX} ${COPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $< 

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o
	${PIN_LD} ${PIN_LDFLAGS} $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} ${PIN_LIBS} $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed *.d *makefile.copy *.exp *.lib

-include *.d

