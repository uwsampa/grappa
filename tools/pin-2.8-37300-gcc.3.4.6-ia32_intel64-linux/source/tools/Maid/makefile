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


EXTRA_LIBS =

OBJS = $(OBJDIR)CallStack.o $(OBJDIR)syscall_names.o $(OBJDIR)Maid.o $(OBJDIR)argv_readparam.o

ifeq ($(TARGET_OS),m)
    # syscall names on Mac yet to be fixed
    TOOL_ROOTS =
else 
    ifeq ($(TARGET),ia32)
       # can't handle 64 bit syscalls
       TOOL_ROOTS = Maid
    endif
endif

##############################################################
#
# build rules
#
##############################################################

all: tools

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

tools: $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TOOL_ROOTS:%=%.test)

testdir:
	+make -C tests


$(OBJDIR)syscall_names.o: syscall_names.cpp syscall_names.H

$(OBJDIR)argv_readparam.o: argv_readparam.cpp argv_readparam.h

## build rules

$(OBJDIR):
	mkdir -p $(OBJDIR)


$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)Maid$(PINTOOL_SUFFIX) : $(OBJS) $(PIN_LIBNAMES)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) $(OBJS) ${LINK_OUT}$@ ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)
## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed *.d 

testmaid: testdir maid
	pin -t ./maid --addrfile=addrfile.txt -- tests/helloworld
