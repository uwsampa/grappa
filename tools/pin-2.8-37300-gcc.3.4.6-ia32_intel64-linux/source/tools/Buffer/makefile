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
    LINKER?=${CXX}
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

TOOL_ROOTS =

ifneq ($(TARGET),ipf)
TOOL_ROOTS += bufferaddr two_buffers allocate mlog_buffer
TOOL_ROOTS += buffer-then buffer-predicate buffererror buffer-lengths
endif

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
TESTS = $(TOOL_ROOTS:%=%.test)
SANITY_TESTS = $(TOOL_ROOTS:%=%.test)



all: tools
tools: $(OBJDIR) $(TOOLS)
test:  $(OBJDIR) $(TOOL_ROOTS:%=%.test)
tests-sanity: $(OBJDIR) $(SANITY_TESTS)



## specific build rules

bufferaddr.test : $(OBJDIR)bufferaddr$(PINTOOL_SUFFIX) bufferaddr.tested bufferaddr.failed
	$(PIN) -xyzzy -mesgon log_buffer -logfile bufferaddr.log -t $< -- $(TESTAPP) makefile $<.makefile.copy
	grep -q -e "Allocated buffer" bufferaddr.log
	rm bufferaddr.failed

two_buffers.test : $(OBJDIR)two_buffers$(PINTOOL_SUFFIX) two_buffers.tested two_buffers.failed
	$(PIN) -xyzzy -mesgon log_buffer -logfile two_buffers.log -t $< -- $(TESTAPP) makefile $<.makefile.copy
	grep -q -e "buffer 1" two_buffers.out
	grep -q -e "buffer 2" two_buffers.out
	rm two_buffers.failed

allocate.test : $(OBJDIR)allocate$(PINTOOL_SUFFIX) allocate.tested allocate.failed
	$(PIN) -xyzzy -mesgon log_buffer -logfile allocate.log -t $< -- $(TESTAPP) makefile $<.makefile.copy
	grep -q -e "found buffer" allocate.log
	rm allocate.failed

buffer-then.test : $(OBJDIR)buffer-then$(PINTOOL_SUFFIX) buffer-then.tested buffer-then.failed
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	rm buffer-then.failed

buffer-predicate.test : $(OBJDIR)buffer-predicate$(PINTOOL_SUFFIX) buffer-predicate.tested buffer-predicate.failed
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	rm buffer-predicate.failed

buffer-lengths.test: $(OBJDIR)buffer-lengths${PINTOOL_SUFFIX} buffer-lengths.tested buffer-lengths.failed
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -e "Test passed" buffer-lengths.out
	rm buffer-lengths.failed

# this is a negative test.  it should fail!
buffererror.test : $(OBJDIR)buffererror$(PINTOOL_SUFFIX) buffererror.tested buffererror.failed
	-$(PIN) -xyzzy -mesgon log_buffer -logfile buffererror.log -t $< -- $(TESTAPP) makefile $<.makefile.copy
	echo "buffererror is a negative test.  Ignore the failure."
	grep -q "IARG_REG_REFERENCE" buffererror.log
	rm buffererror.failed buffererror.log



## build rules

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	${CXX} ${COPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $< 

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o
	${LINKER} ${PIN_LDFLAGS} $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} ${PIN_LIBS} $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.out.* *.tested *.failed *.d *makefile.copy *.exp *.lib *.log

-include *.d

