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
	BIGBINARYFLAGS ?= 
endif

SPECIALRUN = 
NORMALRUN = codeCacheHWCache papiDemo
TOOL_ROOTS = $(SPECIALRUN) $(NORMALRUN)

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

SANITY_TOOLS = 

all: tools 
tools: $(OBJDIR) $(TOOLS) 
test: $(OBJDIR) $(TOOL_ROOTS:%=%.test)
tests-sanity: $(OBJDIR) $(SANITY_TOOLS:%=%.test)

## special testing rules


## build rules
$(OBJDIR):
	mkdir -p $(OBJDIR)


$(OBJDIR)%.o : %.cpp 
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o $(OBJDIR)utils.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) -lpapi $(OBJDIR)utils.o $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed

tidy:
	-rm -rf $(OBJDIR) *.tested *.failed

-include *.d
