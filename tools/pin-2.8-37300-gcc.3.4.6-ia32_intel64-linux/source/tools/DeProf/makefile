##
## PIN tools
##


include ../makefile.gnu.config


DBG=-g
OPT=-O3
CXXFLAGS = -I$(PIN_HOME)/InstLib -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT) -MMD
EXTRA_LIBS =
 
TOOL_ROOTS = deprof
all: tools
SANITY_TOOLS = 

TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

tools: $(TOOLS)
test: $(TOOLS:%=%.test)
tests-sanity: $(SANITY_TOOLS:%=%.test)

derror: derror.o
	$(CXX) -o derror $<
## build rules

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<
$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS) $(EXTRA_LIBS) $(DBG) -fomit-frame-pointer

## cleaning
clean:
	-rm -f *.o $(TOOLS) *.out *.tested *.failed *.d

%.test : % %.tested %.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	rm $<.makefile.copy; rm $<.failed

-include *.d
