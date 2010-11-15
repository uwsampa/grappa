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
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
endif

ifneq ($(ENABLE_VS), 1)
    VS_FLAG = -xyzzy -virtual_segments 1
else
    VS_FLAG =
endif


ifeq ($(TARGET_OS),l)
    ifeq ($(TARGET),ia32)
        APP_ROOTS += modify_ldt set_thread_area1 set_thread_area2 set_thread_area3 load_far_pointer fixed_base_reg
        TESTS += modify_ldt set_thread_area1 set_thread_area2 set_thread_area3 load_far_pointer fixed_base_reg
    endif
    ifeq ($(TARGET),ia32e)
        APP_ROOTS += modify_ldt load_far_pointer_32e
        TESTS += modify_ldt load_far_pointer_32e
    endif
endif

APPS = $(APP_ROOTS:%=$(OBJDIR)%)

all: $(OBJDIR) $(APPS) 
test: $(OBJDIR) $(TESTS:=.test)
tests-sanity: test


#
# Rules to build the applications
#

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)modify_ldt: modify_ldt.cpp
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ $< $(APP_PTHREAD) $(APP_CXXLINK_FLAGS)

$(OBJDIR)set_thread_area1: set_thread_area1.cpp
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ $< $(APP_PTHREAD) $(APP_CXXLINK_FLAGS)

$(OBJDIR)set_thread_area2: set_thread_area2.cpp
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ $< $(APP_PTHREAD) $(APP_CXXLINK_FLAGS)

$(OBJDIR)set_thread_area3: set_thread_area3.cpp
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)load_far_pointer: load_far_pointer.cpp load_far_pointer.s
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ load_far_pointer.cpp load_far_pointer.s $(APP_CXXLINK_FLAGS)

$(OBJDIR)load_far_pointer_32e: load_far_pointer_32e.cpp load_far_pointer_32e.s
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ load_far_pointer_32e.cpp load_far_pointer_32e.s $(APP_CXXLINK_FLAGS)

$(OBJDIR)fixed_base_reg: fixed_base_reg.cpp movs.s
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) ${OUTOPT} $@ fixed_base_reg.cpp movs.s $(APP_CXXLINK_FLAGS)



#
# Rules to run the tests.
#

modify_ldt.test: %.test : $(OBJDIR)modify_ldt %.failed %.tested
	./$< > $*.out
	$(PIN)  $(VS_FLAG) -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out

set_thread_area1.test: %.test : $(OBJDIR)set_thread_area1 %.failed %.tested
	./$< > $*.out
	$(PIN) $(VS_FLAG) -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out

set_thread_area2.test: %.test : $(OBJDIR)set_thread_area2 %.failed %.tested
	./$< > $*.out
	$(PIN) $(VS_FLAG)  -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out

set_thread_area3.test: %.test : $(OBJDIR)set_thread_area3 %.failed %.tested
	./$< > $*.out
	$(PIN) $(VS_FLAG) -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out

load_far_pointer.test: %.test : $(OBJDIR)load_far_pointer %.failed %.tested
	./$< > $*.out
	$(PIN) $(VS_FLAG) -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out
    
load_far_pointer_32e.test: %.test : $(OBJDIR)load_far_pointer_32e %.failed %.tested
	./$< > $*.out
	$(PIN) $(VS_FLAG) -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out

fixed_base_reg.test: %.test : $(OBJDIR)fixed_base_reg %.failed %.tested
	./$< > $*.out
	$(PIN) $(VS_FLAG) -- ./$< >  $*.pin.out 2>&1
	$(PIN_DIFF) $*.out $*.pin.out
	rm $(@:.test=.failed) $*.out $*.pin.out


clean:
	rm -rf $(OBJDIR) $(TESTS:=.tested) $(TESTS:=.failed) $(TOOLS:=.out) $(TOOLS:=.pin.out)
