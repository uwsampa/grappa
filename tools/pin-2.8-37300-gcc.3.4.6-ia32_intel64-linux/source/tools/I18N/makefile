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

APPS = 

ifeq ($(TARGET_OS),w)
	ifeq ($(TARGET_COMPILER),ms)
		LOCAL_OS=win
		EXE_POSTFIX=.exe
		TOOL_ROOTS += i18n_tool
		TESTS += win_unicode
		APP_CXXDEBUG = /Zi
		PIN=$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64)
	endif
endif

ifeq ($(TARGET_OS),l)
	LOCAL_OS=lin
	EXE_POSTFIX=
	TOOL_ROOTS += i18n_tool
	CXXVER = $(shell $(CXX) --version | head -1 | cut -d' ' -f3)
	ifneq ($(CXXVER),3.2.3)
		TESTS += lin_unicode
	endif
	APP_CXXDEBUG = -g
	APPS += $(LOCAL_OS)_unicode_test_launcher lin_unicode
endif

ifeq ($(TARGET_OS),b)
	LOCAL_OS=lin
	EXE_POSTFIX=
	TOOL_ROOTS += i18n_tool
	TESTS += lin_unicode
	APP_CXXDEBUG = -g
	APPS += $(LOCAL_OS)_unicode_test_launcher lin_unicode
endif

#
# Global build targets
#
TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

all: $(OBJDIR) $(TOOLS) $(APPS:%=$(OBJDIR)%)
test: $(OBJDIR) $(TESTS:%=%.test)
tests-sanity: test

#
# Rules to build the tools
#
$(OBJDIR):
	mkdir -p $(OBJDIR)


$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

#
# Rules to build the tests
#

# Common

# Either lin_unicode.test or win_unicode.test
${LOCAL_OS}_unicode.test: %.test : $(OBJDIR)%_test_launcher %_get_app %.tested %.failed $(OBJDIR)i18n_tool$(PINTOOL_SUFFIX)
	touch $*.out; rm $*.out
	./$(OBJDIR)${LOCAL_OS}_unicode_test_launcher${EXE_POSTFIX} test $(PIN) -t $(OBJDIR)i18n_tool$(PINTOOL_SUFFIX) -uni_param ParamName -- ./$(OBJDIR)${LOCAL_OS}_unicode${EXE_POSTFIX}
	$(PIN_DIFF) $*.out $*.reference
	$(PIN_DIFF) i18n_tool.out ${LOCAL_OS}_i18n_tool.reference
	rm $*.failed  $*.out i18n_tool.out
	
# Either lin_unicode_test_launcher or win_unicode_test_launcher	
$(OBJDIR)${LOCAL_OS}_unicode_test_launcher : ${LOCAL_OS}_unicode_test_launcher.cpp
	$(CXX) $(DBG) ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

# Windows specific

# Can't compile with /Zi flag on ssh sesion - take the PRE-compiled application		
win_unicode_get_app:
	cp win_unicode${EXE_POSTFIX}-${TARGET} $(OBJDIR)win_unicode${EXE_POSTFIX}
	cp win_unicode.pdb-${TARGET} $(OBJDIR)win_unicode.pdb

$(OBJDIR)win_unicode : win_unicode.cpp win_unicode_test_launcher
	echo ${APP_CXXDEBUG}
	./$(OBJDIR)win_unicode_test_launcher${EXE_POSTFIX} make $(CXX) ${APP_CXXDEBUG} ${APP_CXXFLAGS2} ${OUTEXE}$(OBJDIR)win_unicode win_unicode.cpp
	cp win_unicode${EXE_POSTFIX} $(OBJDIR)win_unicode${EXE_POSTFIX}-${TARGET}
	cp win_unicode.pdb $(OBJDIR)win_unicode.pdb-${TARGET}
	rm -f win_unicode${EXE_POSTFIX} win_unicode.pdb win_unicode.ilk 
  
# Linux specific

lin_unicode_get_app: $(OBJDIR)lin_unicode 
 
$(OBJDIR)lin_unicode : lin_unicode.cpp $(OBJDIR)lin_unicode_test_launcher
	./$(OBJDIR)lin_unicode_test_launcher${EXE_POSTFIX} make $(CXX) ${APP_CXXDEBUG} ${APP_CXXFLAGS} ${APP_CXXFLAGS2} ${OUTEXE}$(OBJDIR)lin_unicode lin_unicode.cpp
	
## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed
