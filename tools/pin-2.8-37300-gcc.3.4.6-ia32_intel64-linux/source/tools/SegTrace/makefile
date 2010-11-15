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


ifeq ($(TARGET_OS),w)
    ifeq ($(TARGET),ia32)
        APPS  = hello
        TOOL_ROOTS = segtrace
        TESTS = trace_hello
    endif
    ifeq ($(TARGET),ia32e)
        APPS  = hello
        TOOL_ROOTS = segtrace
        TESTS = trace_hello
    endif
else
  ifeq ($(TARGET_OS),b)
    # FreeBSD
    APPS = thread_bsd
    TOOL_ROOTS = segtrace_bsd
    TESTS = trace_thread_bsd
  else
	# Linux and MAC
    ifeq ($(TARGET),ia32)
        APPS  = thread
        TOOL_ROOTS = segtrace
        TESTS = trace_thread
    endif
    ifeq ($(TARGET),ia32e)
        APPS  = thread
        TOOL_ROOTS = segtrace
        TESTS = trace_thread
    endif
  endif
endif

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

all: $(APPS:%=$(OBJDIR)%) $(TOOLS)
test: $(TESTS:=.test)
tests-sanity: test

$(APPS:%=$(OBJDIR)%) $(TOOLS): $(OBJDIR)make-directory

$(OBJDIR)make-directory:
	mkdir -p $(OBJDIR)
	touch  $(OBJDIR)make-directory
$(OBJDIR):
	mkdir -p $(OBJDIR)



#
# Rules to build the applications
#
$(OBJDIR)thread: thread.c
	$(CC) $(APP_CXXFLAGS) $(OUTEXE)$@ $< $(APP_PTHREAD)

$(OBJDIR)hello: hello.c
	$(CC) $(APP_CXXFLAGS) $(OUTEXE)$@ $<

$(OBJDIR)thread_bsd: thread_bsd.c
	$(CC) $(APP_CXXFLAGS) $(OUTEXE)$@ $< $(APP_PTHREAD)


#
# Rules to build the tools
#
$(OBJDIR)%.o : %.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ $< 

$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	$(PIN_LD) $(PIN_LDFLAGS) $(LINK_DEBUG) $(LINK_OUT)$@ $< $(PIN_LPATHS) $(PIN_LIBS) $(DBG)


#
# Rules to run the tests.
#
trace_thread.test: trace_thread.tested trace_thread.failed $(OBJDIR)segtrace$(PINTOOL_SUFFIX) $(OBJDIR)thread
	$(PIN) -t $(OBJDIR)segtrace$(PINTOOL_SUFFIX) -- ./$(OBJDIR)thread
	rm $(@:.test=.failed)

trace_hello.test: trace_hello.tested trace_hello.failed $(OBJDIR)segtrace$(PINTOOL_SUFFIX) $(OBJDIR)hello
	$(PIN) -t $(OBJDIR)segtrace$(PINTOOL_SUFFIX) -- ./$(OBJDIR)hello
	rm $(@:.test=.failed)

trace_thread_bsd.test: trace_thread_bsd.tested trace_thread_bsd.failed $(OBJDIR)segtrace_bsd$(PINTOOL_SUFFIX) $(OBJDIR)thread_bsd
	$(PIN) -t $(OBJDIR)segtrace_bsd$(PINTOOL_SUFFIX) -- ./$(OBJDIR)thread_bsd
	rm $(@:.test=.failed)

clean:
	rm -rf $(OBJDIR) $(TESTS:=.tested) $(TESTS:=.failed) *.out pin.log
