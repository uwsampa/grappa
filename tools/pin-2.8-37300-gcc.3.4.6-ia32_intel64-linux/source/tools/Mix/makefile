##
## PIN tools
##

## mix-mt: handles variable number of threads.

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
    DEBUG?=0
    include ../makefile.ms.config
endif

##############################################################
#
# build rules
#
##############################################################

APP_ROOTS       = marker-example
ifeq ($(TARGET_OS),l)
	APP_ROOTS += test-mt
endif
ifeq ($(TARGET_HOST),ipf)
	APP_ROOTS += fp
endif

TOOL_ROOTS = mix-mt
TESTS      = mix-mt.test
ifeq ($(TARGET_OS),l)
     # uses pthreads for the user app (so linux only)
    CXXVER = $(shell $(CXX) --version | head -1 | cut -d' ' -f3 | cut -d"." -f1,2)

    # disable this test when the compiler is GCC 3.2.X or 3.3.X.
    # since there is no "and" for ifeq/ifneq, use subst to find bad compilers
    CXXCHECK:=$(subst 3.2,bad,$(CXXVER))
    CXXCHECK:=$(subst 3.3,bad,$(CXXCHECK))
    ifneq ($(CXXCHECK),bad)
        TESTS += test-mt.test 
    endif
    ifeq ($(TARGET),ia32e)
       TESTS += mix-length.test 
    endif
endif
ifeq ($(TARGET_HOST),ipf)
      TESTS += fp.test
endif

ifeq ($(TARGET_OS),b)
    APP_ROOTS += test-mt
	TESTS += test-mt.test
endif

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
SANITY_TESTS =  $(TESTS)
APPS = $(APP_ROOTS:%=$(OBJDIR)%)

all: $(OBJDIR) $(APPS) $(TOOLS)
tools: $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TESTS)
tests-sanity: $(OBJDIR) $(TESTS)


## build rules

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(OBJDIR)%.o : %.s
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)assy-support-intel64.obj: assy-support-intel64.s
	ml64 $(NO_LOGO) /c /Fo$@ $<
$(OBJDIR)assy-support-ia32.obj: assy-support-ia32.s
	ml $(NO_LOGO) /c /Fo$@ $<

ifeq ($(TARGET_HOST),ipf)
  ASSY_FILES=
else
  ifeq ($(TARGET_OS),w)
     ASSY_FILES=$(OBJDIR)assy-support-$(TARGET_LONG).obj
  else
     ASSY_FILES=
  endif
endif

$(OBJDIR)mix-mt$(PINTOOL_SUFFIX) : $(OBJDIR)mix-mt.o $(ASSY_FILES) $(PIN_LIBNAMES)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $(OBJDIR)mix-mt.o $(ASSY_FILES) ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)marker-example: marker-example.cpp
	${CXX} $(NO_LOGO) $(APP_CXXFLAGS) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)
$(OBJDIR)test-mt: test-mt.cpp
	${CXX} $(NO_LOGO) $(APP_CXXFLAGS) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(APP_PTHREAD)
$(OBJDIR)fp: fp.c
	${CXX} $(NO_LOGO) $(APP_CXXFLAGS) -O3 -fno-inline ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) 


mix-mt.test: %.test : $(OBJDIR)mix-mt$(PINTOOL_SUFFIX) $(OBJDIR)marker-example %.tested %.failed 
	$(PIN) -t ./$< -category -start_address marker_start_counting:repeat -stop_address marker_stop_counting:repeat -zero_stats_address marker_zero_stats:repeat -emit_stats_address marker_emit_stats:repeat -o mix.out.1 -- ./$(OBJDIR)marker-example
	rm $(@:.test=.failed)

fp.test: $(OBJDIR)mix-mt$(PINTOOL_SUFFIX) $(OBJDIR)fp fp.tested fp.failed
	$(PIN) -xyzzy -region_max_bbls 1 -t ./$<  -zero_stats_address marker_zero_stats -start_address marker_start_counting -stop_address marker_stop_counting -o fp.out -- ./$(OBJDIR)fp
	grep FMA_D fp.out | head -3 > fp.out2
	diff fp.out2 fp.ref
	rm fp.failed

# MULTITHREADED TEST: mix-mt handles a variable number of threads 
test-mt.test: $(OBJDIR)mix-mt$(PINTOOL_SUFFIX) $(OBJDIR)test-mt test-mt.tested test-mt.failed  
	$(PIN) -t ./$<  -start_address marker_start_counting:repeat -stop_address marker_stop_counting:repeat -zero_stats_address marker_zero_stats:repeat -emit_stats_address marker_emit_stats:repeat -o mix.out.4 -- ./$(OBJDIR)test-mt
	rm test-mt.failed


# I allow some fuzz in the expected length because sometime we see ~960 and 
# sometimes 1000. This is because the length counter counts blocks and
# that results in approximate counting.
mix-length.test: $(OBJDIR)mix-mt$(PINTOOL_SUFFIX)  mix-length.tested mix-length.failed  
	$(PIN) -t ./$<  -length 1000 -o mix.out.5 -- /bin/ls
	test `tail mix.out.5 | grep total |  gawk '{print $$2}'` -lt 1010
	test `tail mix.out.5 | grep total |  gawk '{print $$2}'` -gt  960
	rm mix-length.failed



## cleaning
clean:
	-rm -rf $(OBJDIR) *.tested *.failed *.out* 

-include *.d

