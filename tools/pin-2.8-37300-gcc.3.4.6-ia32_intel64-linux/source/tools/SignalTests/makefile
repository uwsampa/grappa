##
## Tests to validate Pin's emulation of application signal handling
##


include ../makefile.gnu.config


#
# None of these tests run on Windows or Mac since signals are not supported there.
#

APPS_lb        = simple nestmask syncasyncapp fpcheck maskcheck segv blockloop exithandler altstack errcheck fatal sigcont \
                 sigenableapp sigprocmask_short sig64 fatalsync blockingsyscall futex asyncfpe fatal_block_sync \
                 suspendmask efault shortmask segv_for_regbindings_cache_test resetsegv \
                 resetcont handle-usr1 async_multithreadingapp mtstress
APPS_ipf_l     = faultcheck-ia64 natcheck
APPS_x86_lb    = segv_on_fetch replay-signal-app
APPS_ia32_lb   = xmmcheck_ia32 xmmfaultcheck_ia32 faultcheck-ia32 faultcheck_flags_ia32 movsbtest_ia32 hlt sigaltstack \
                 dfcheck_ia32 fptags_ia32 bad-stack-ia32
APPS_ia32e_lb  = xmmcheck_ia32e xmmfaultcheck_ia32e faultcheck-intel64 movsbtest_ia32e dfcheck_ia32e bad-stack-intel64

TOOLS_lb       = syncasynctool exittool interceptsegv logtool intercepttool sigflowcheck spilltool fastsigchecktool sigenable \
                 interceptall faulttool action-pending interceptor_for_regbindings_cache_test resetsegv-safecopy \
				 deprecated-intercept reporter async_multithreadingtool
TOOLS_x86_lb   = faultcheck_flags_tool1 faultcheck_flags_tool2 faultcheck_flags_tool3 replay-signal-tool
TOOLS_x86_l    = raise-exception-tool

TESTS_lb       = nestmask altstack errcheck fatallog sigcont fastsigcheck sigenable interceptall sigprocmask_short sig64 \
                 faulttool fatalsync blockingsyscall futex fatal_block_sync suspendmask efault maskcheck \
                 action-pending simpleasync simplesync fpcheck interceptsegv interceptsegv_for_regbindings_cache intercept \
                 sigflowcheck1 sigflowcheck2 syncasync resetsegv resetcont resetsegv-safecopy log deprecated-intercept async_multithreading \
                 mtstress
TESTS_l        = asyncfpe shortmask
TESTS_ifp_l    = faultcheck-ia64 natcheck
TESTS_x86_lb   = segv_on_fetch replay-signal bad-stack-noalt bad-stack-alt bad-stack-sigret
TESTS_ia32_lb  = xmmcheck_ia32 xmmfaultcheck_ia32 xmmfaultcheckspill_ia32 faultcheck-ia32 faultcheck_flags_ia32_1 \
                 faultcheck_flags_ia32_2 movsbtest_ia32 hlt sigaltstack dfcheck_ia32 fptags_ia32
TESTS_ia32e_lb = xmmcheck_ia32e xmmfaultcheck_ia32e faultcheck-intel64 movsbtest_ia32e dfcheck_ia32e
TESTS_ia32_l   = raise-exception-ia32
TESTS_ia32e_l  = raise-exception-intel64


# The following tests are disabled.  See Mantis #443.
#
#    sigsuspend, double, recursive, setcontext, sigusr2


apps_ia32_l   = $(APPS) $(APPS_l) $(APPS_lb) $(APPS_x86) $(APPS_x86_l) $(APPS_x86_lb) $(APPS_ia32) $(APPS_ia32_l) $(APPS_ia32_lb)
apps_ia32e_l  = $(APPS) $(APPS_l) $(APPS_lb) $(APPS_x86) $(APPS_x86_l) $(APPS_x86_lb) $(APPS_ia32e) $(APPS_ia32e_l) $(APPS_ia32e_lb)
apps_ipf_l    = $(APPS) $(APPS_l) $(APPS_lb) $(APPS_ipf) $(APPS_ipf_l) $(APPS_ipf_lb)
apps_ia32_b   = $(APPS) $(APPS_b) $(APPS_lb) $(APPS_x86) $(APPS_x86_b) $(APPS_x86_lb) $(APPS_ia32) $(APPS_ia32_b) $(APPS_ia32_lb)
apps_ia32e_b  = $(APPS) $(APPS_b) $(APPS_lb) $(APPS_x86) $(APPS_x86_b) $(APPS_x86_lb) $(APPS_ia32e) $(APPS_ia32e_b) $(APPS_ia32e_lb)
tools_ia32_l  = $(TOOLS) $(TOOLS_l) $(TOOLS_lb) $(TOOLS_x86) $(TOOLS_x86_l) $(TOOLS_x86_lb) $(TOOLS_ia32) $(TOOLS_ia32_l) $(TOOLS_ia32_lb)
tools_ia32e_l = $(TOOLS) $(TOOLS_l) $(TOOLS_lb) $(TOOLS_x86) $(TOOLS_x86_l) $(TOOLS_x86_lb) $(TOOLS_ia32e) $(TOOLS_ia32e_l) $(TOOLS_ia32e_lb)
tools_ipf_l   = $(TOOLS) $(TOOLS_l) $(TOOLS_lb) $(TOOLS_ipf) $(TOOLS_ipf_l) $(TOOLS_ipf_lb)
tools_ia32_b  = $(TOOLS) $(TOOLS_b) $(TOOLS_lb) $(TOOLS_x86) $(TOOLS_x86_b) $(TOOLS_x86_lb) $(TOOLS_ia32) $(TOOLS_ia32_b) $(TOOLS_ia32_lb)
tools_ia32e_b = $(TOOLS) $(TOOLS_b) $(TOOLS_lb) $(TOOLS_x86) $(TOOLS_x86_b) $(TOOLS_x86_lb) $(TOOLS_ia32e) $(TOOLS_ia32e_b) $(TOOLS_ia32e_lb)
tests_ia32_l  = $(TESTS) $(TESTS_l) $(TESTS_lb) $(TESTS_x86) $(TESTS_x86_l) $(TESTS_x86_lb) $(TESTS_ia32) $(TESTS_ia32_l) $(TESTS_ia32_lb)
tests_ia32e_l = $(TESTS) $(TESTS_l) $(TESTS_lb) $(TESTS_x86) $(TESTS_x86_l) $(TESTS_x86_lb) $(TESTS_ia32e) $(TESTS_ia32e_l) $(TESTS_ia32e_lb)
tests_ipf_l   = $(TESTS) $(TESTS_l) $(TESTS_lb) $(TESTS_ipf) $(TESTS_ipf_l) $(TESTS_ipf_lb)
tests_ia32_b  = $(TESTS) $(TESTS_b) $(TESTS_lb) $(TESTS_x86) $(TESTS_x86_b) $(TESTS_x86_lb) $(TESTS_ia32) $(TESTS_ia32_b) $(TESTS_ia32_lb)
tests_ia32e_b = $(TESTS) $(TESTS_b) $(TESTS_lb) $(TESTS_x86) $(TESTS_x86_b) $(TESTS_x86_lb) $(TESTS_ia32e) $(TESTS_ia32e_b) $(TESTS_ia32e_lb)

apps  = $(apps_$(TARGET)_$(TARGET_OS))
tools = $(tools_$(TARGET)_$(TARGET_OS))
tests = $(tests_$(TARGET)_$(TARGET_OS)) dummy


all: $(apps:%=$(OBJDIR)%) $(tools:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
test: $(tests:=.test)
tests-sanity: test

$(apps:%=$(OBJDIR)%) $(tools:%=$(OBJDIR)%): $(OBJDIR)make-directory

$(OBJDIR)make-directory:
	mkdir -p $(OBJDIR)
	touch  $(OBJDIR)make-directory
$(OBJDIR):
	mkdir -p $(OBJDIR)



#
# Rules to build the applications
#
$(OBJDIR)nestmask: nestmask.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)segv: segv.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<
	
$(OBJDIR)segv_for_regbindings_cache_test: segv_for_regbindings_cache_test.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)syncasyncapp: syncasyncapp.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)async_multithreadingapp: async_multithreadingapp.c
	$(CC) $(APP_CXXFLAGS) -o $@ $< -lpthread 

$(OBJDIR)fpcheck: fpcheck.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $< -lm

$(OBJDIR)xmmcheck_ia32: xmmcheck.c copywithxmm_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ xmmcheck.c copywithxmm_ia32.s

$(OBJDIR)xmmcheck_ia32e: xmmcheck.c copywithxmm_ia32e.s
	$(CC) $(APP_CXXFLAGS) -o $@ xmmcheck.c copywithxmm_ia32e.s

$(OBJDIR)xmmfaultcheck_ia32: xmmfaultcheck.c copywithxmm_ia32.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32 -o $@ xmmfaultcheck.c copywithxmm_ia32.s

$(OBJDIR)xmmfaultcheck_ia32e: xmmfaultcheck.c copywithxmm_ia32e.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32E -o $@ xmmfaultcheck.c copywithxmm_ia32e.s

$(OBJDIR)blockloop: blockloop.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)maskcheck: maskcheck.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)exithandler: exithandler.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)simple: simple.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)altstack: altstack.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)faultcheck-ia32: faultcheck.c faultcheck-x86.c faultcheck-ia32-asm.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32 -o $@ faultcheck.c faultcheck-x86.c faultcheck-ia32-asm.s

$(OBJDIR)faultcheck-intel64: faultcheck.c faultcheck-x86.c faultcheck-intel64-asm.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32E -o $@ faultcheck.c faultcheck-x86.c faultcheck-intel64-asm.s

$(OBJDIR)faultcheck-ia64: faultcheck.c faultcheck-ia64.c faultcheck_ipf_asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ faultcheck.c faultcheck-ia64.c faultcheck_ipf_asm.s

$(OBJDIR)faultcheck_flags_ia32: faultcheck_flags.c  faultcheck_flags_ia32_asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ faultcheck_flags.c  faultcheck_flags_ia32_asm.s

$(OBJDIR)errcheck: errcheck.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)natcheck: natcheck.c natcheck_asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ natcheck.c natcheck_asm.s

$(OBJDIR)movsbtest_ia32: movsbtest.c movsbtest_ia32.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32 -o $@ movsbtest.c movsbtest_ia32.s

$(OBJDIR)movsbtest_ia32e: movsbtest.c movsbtest_ia32e.s
	$(CC) $(APP_CXXFLAGS) -DTARGET_IA32E -o $@ movsbtest.c movsbtest_ia32e.s

$(OBJDIR)double: double.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)hlt: hlt.c
	$(CC) $(APP_CXXFLAGS) -O0 -o $@ $<

$(OBJDIR)recursive: recursive.c
	$(CC) $(APP_CXXFLAGS) -g -O -o $@ $<

$(OBJDIR)setcontext: setcontext.c
	$(CC) $(APP_CXXFLAGS)  -g -O -o $@ $<

$(OBJDIR)sigaltstack: sigaltstack.c
	$(CC) $(APP_CXXFLAGS) -g -O $(APP_PTHREAD) -o $@ $<

$(OBJDIR)sigsuspend: sigsuspend.c
	$(CC) $(APP_CXXFLAGS) -g -O $(APP_PTHREAD) -o $@ $<

$(OBJDIR)sigusr2: sigusr2.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)fatal: fatal.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)sigcont: sigcont.c
	$(CC) $(APP_CXXFLAGS) -O -o $@ $<

$(OBJDIR)sigenableapp: sigenableapp.c sigenableapp2.c
	$(CC) $(APP_CXXFLAGS) -o $@ sigenableapp.c sigenableapp2.c

$(OBJDIR)sigprocmask_short: sigprocmask_short.cpp
	$(CXX) $(PIN_CXXFLAGS) -o $@ sigprocmask_short.cpp

$(OBJDIR)sig64: sig64.cpp
	$(CXX) $(APP_CXXFLAGS) -o $@ sig64.cpp

$(OBJDIR)dfcheck_ia32: dfcheck.c dfcheck_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ dfcheck.c dfcheck_ia32.s

$(OBJDIR)dfcheck_ia32e: dfcheck.c dfcheck_ia32e.s
	$(CC) $(APP_CXXFLAGS) -o $@ dfcheck.c dfcheck_ia32e.s

$(OBJDIR)fatalsync: fatalsync.c
	$(CC) $(APP_CXXFLAGS) -o $@ fatalsync.c

$(OBJDIR)blockingsyscall: blockingsyscall.c
	$(CC) $(APP_CXXFLAGS) -o $@ blockingsyscall.c

$(OBJDIR)futex: futex.c
	$(CC) $(APP_CXXFLAGS) -o $@ futex.c $(APP_PTHREAD)

$(OBJDIR)fptags_ia32: fptags.c fptags_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ fptags.c fptags_ia32.s

$(OBJDIR)asyncfpe: asyncfpe.c
	$(CC) $(APP_CXXFLAGS) -o $@ asyncfpe.c

$(OBJDIR)segv_on_fetch: segv_on_fetch.c
	$(CC) $(APP_CXXFLAGS) -o $@ segv_on_fetch.c

$(OBJDIR)fatal_block_sync: fatal_block_sync.c
	$(CC) $(APP_CXXFLAGS) -o $@ fatal_block_sync.c

$(OBJDIR)suspendmask: suspendmask.c
	$(CC) $(APP_CXXFLAGS) -o $@ suspendmask.c

$(OBJDIR)efault: efault.c
	$(CC) $(APP_CXXFLAGS) -o $@ efault.c

$(OBJDIR)shortmask: shortmask.c
	$(CC) $(APP_CXXFLAGS) -o $@ shortmask.c

$(OBJDIR)replay-signal-app: replay-signal-app.c
	$(CC) $(APP_CXXFLAGS) -o $@ replay-signal-app.c

$(OBJDIR)resetsegv: resetsegv.c
	$(CC) $(APP_CXXFLAGS) -o $@ resetsegv.c

$(OBJDIR)resetcont: resetcont.c
	$(CC) $(APP_CXXFLAGS) -o $@ resetcont.c

$(OBJDIR)handle-usr1: handle-usr1.c
	$(CC) $(APP_CXXFLAGS) -o $@ handle-usr1.c

$(OBJDIR)bad-stack-$(TARGET_LONG): bad-stack.c bad-stack-$(TARGET_LONG)-asm.s
	$(CC) $(APP_CXXFLAGS) -o $@ bad-stack.c -x assembler-with-cpp bad-stack-$(TARGET_LONG)-asm.s

$(OBJDIR)mtstress: mtstress.cpp
	$(CXX) $(APP_CXXFLAGS) -o $@ mtstress.cpp $(APP_PTHREAD)


#
# Rules to build the object files
#
$(OBJDIR)%.o: %.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<

#
# Rules to build the tools
#
$(OBJDIR)interceptsegv$(PINTOOL_SUFFIX): $(OBJDIR)interceptsegv.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)interceptor_for_regbindings_cache_test$(PINTOOL_SUFFIX): $(OBJDIR)interceptor_for_regbindings_cache_test.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)syncasynctool$(PINTOOL_SUFFIX): $(OBJDIR)syncasynctool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)async_multithreadingtool$(PINTOOL_SUFFIX): $(OBJDIR)async_multithreadingtool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)logtool$(PINTOOL_SUFFIX): $(OBJDIR)logtool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)intercepttool$(PINTOOL_SUFFIX): $(OBJDIR)intercepttool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX): $(OBJDIR)sigflowcheck.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)exittool$(PINTOOL_SUFFIX): $(OBJDIR)exittool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)spilltool$(PINTOOL_SUFFIX): $(OBJDIR)spilltool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)fastsigchecktool$(PINTOOL_SUFFIX): $(OBJDIR)fastsigchecktool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)sigenable$(PINTOOL_SUFFIX): $(OBJDIR)sigenable.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)interceptall$(PINTOOL_SUFFIX): $(OBJDIR)interceptall.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faulttool$(PINTOOL_SUFFIX): $(OBJDIR)faulttool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faultcheck_flags_tool1$(PINTOOL_SUFFIX): $(OBJDIR)faultcheck_flags_tool1.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faultcheck_flags_tool2$(PINTOOL_SUFFIX): $(OBJDIR)faultcheck_flags_tool2.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)faultcheck_flags_tool3$(PINTOOL_SUFFIX): $(OBJDIR)faultcheck_flags_tool3.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX): $(OBJDIR)raise-exception-tool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)action-pending$(PINTOOL_SUFFIX): $(OBJDIR)action-pending.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)replay-signal-tool$(PINTOOL_SUFFIX): $(OBJDIR)replay-signal-tool.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)resetsegv-safecopy$(PINTOOL_SUFFIX): $(OBJDIR)resetsegv-safecopy.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)deprecated-intercept.o: deprecated-intercept.cpp $(OBJDIR)make-directory
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) -DPIN_DEPRECATED_WARNINGS=0 -o $@ $<
$(OBJDIR)deprecated-intercept$(PINTOOL_SUFFIX): $(OBJDIR)deprecated-intercept.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)

$(OBJDIR)reporter$(PINTOOL_SUFFIX): $(OBJDIR)reporter.o
	$(CXX) $(PIN_LDFLAGS) -o $@ $< $(PIN_LIBS)


#
# Rules to run the tests.
#
nestmask.test: $(OBJDIR)nestmask $(OBJDIR)nestmask.tested $(OBJDIR)nestmask.failed 
	$(PIN) -- ./$<
	rm $(OBJDIR)$(@:.test=.failed)

interceptsegv.test: $(OBJDIR)interceptsegv$(PINTOOL_SUFFIX) $(OBJDIR)segv $(OBJDIR)interceptsegv.tested $(OBJDIR)interceptsegv.failed
	$(PIN) -t $(OBJDIR)interceptsegv$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.toolout) -- ./$(OBJDIR)segv
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.toolout)
	rm $(OBJDIR)$(@:.test=.failed)

interceptsegv_for_regbindings_cache.test: $(OBJDIR)interceptsegv_for_regbindings_cache.tested $(OBJDIR)interceptsegv_for_regbindings_cache.failed $(OBJDIR)interceptor_for_regbindings_cache_test$(PINTOOL_SUFFIX) $(OBJDIR)segv_for_regbindings_cache_test
	$(PIN) -xyzzy -statistic -logfile $(OBJDIR)interceptsegv_for_regbindings_cache.log -t $(OBJDIR)interceptor_for_regbindings_cache_test$(PINTOOL_SUFFIX) -- ./$(OBJDIR)segv_for_regbindings_cache_test
	test `grep regbindings_cache_hits $(OBJDIR)interceptsegv_for_regbindings_cache.log | grep -c 0` -eq 0
	rm $(OBJDIR)$(@:.test=.failed)

syncasync.test: $(OBJDIR)syncasync.tested $(OBJDIR)syncasync.failed $(OBJDIR)syncasynctool$(PINTOOL_SUFFIX) $(OBJDIR)syncasyncapp
	$(PIN) -t $(OBJDIR)syncasynctool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)syncasyncapp
	rm $(OBJDIR)$(@:.test=.failed)

async_multithreading.test: $(OBJDIR)async_multithreading.tested $(OBJDIR)async_multithreading.failed $(OBJDIR)async_multithreadingtool$(PINTOOL_SUFFIX) $(OBJDIR)async_multithreadingapp
	$(PIN) -t $(OBJDIR)async_multithreadingtool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)async_multithreadingapp
	rm $(OBJDIR)$(@:.test=.failed)

fpcheck.test: $(OBJDIR)fpcheck $(OBJDIR)fpcheck.tested $(OBJDIR)fpcheck.failed 
	$(PIN) -- ./$<
	rm $(OBJDIR)$(@:.test=.failed)

xmmcheck_ia32.test: $(OBJDIR)xmmcheck_ia32 $(OBJDIR)xmmcheck_ia32.tested $(OBJDIR)xmmcheck_ia32.failed 
	$(PIN) -- ./$<
	rm $(OBJDIR)$(@:.test=.failed)

xmmcheck_ia32e.test: $(OBJDIR)xmmcheck_ia32e $(OBJDIR)xmmcheck_ia32e.tested $(OBJDIR)xmmcheck_ia32e.failed 
	$(PIN) -- ./$<
	rm $(OBJDIR)$(@:.test=.failed)

xmmfaultcheck_ia32.test: $(OBJDIR)xmmfaultcheck_ia32.tested $(OBJDIR)xmmfaultcheck_ia32.failed $(OBJDIR)xmmfaultcheck_ia32
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(OBJDIR)$(@:.test=.failed)

xmmfaultcheckspill_ia32.test: $(OBJDIR)xmmfaultcheckspill_ia32.tested $(OBJDIR)xmmfaultcheckspill_ia32.failed $(OBJDIR)xmmfaultcheck_ia32 $(OBJDIR)spilltool$(PINTOOL_SUFFIX)
	$(PIN) -t $(OBJDIR)spilltool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)xmmfaultcheck_ia32
	rm $(OBJDIR)$(@:.test=.failed)

xmmfaultcheck_ia32e.test: $(OBJDIR)xmmfaultcheck_ia32e.tested $(OBJDIR)xmmfaultcheck_ia32e.failed $(OBJDIR)xmmfaultcheck_ia32e
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(OBJDIR)$(@:.test=.failed)

log.test: $(OBJDIR)log.tested $(OBJDIR)log.failed $(OBJDIR)logtool$(PINTOOL_SUFFIX) $(OBJDIR)nestmask
	$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o $(OBJDIR)log.out -- ./$(OBJDIR)nestmask
	$(PIN_CMP) $(OBJDIR)log.out log.reference
	rm $(OBJDIR)$(@:.test=.failed)

intercept.test: $(OBJDIR)intercept.tested $(OBJDIR)intercept.failed $(OBJDIR)intercepttool$(PINTOOL_SUFFIX) $(OBJDIR)blockloop
	rm -f $(OBJDIR)intercept.out; touch $(OBJDIR)intercept.out; \
        $(PIN) -pause_tool 1 -t $(OBJDIR)intercepttool$(PINTOOL_SUFFIX) -s 1 -c 2 -- ./$(OBJDIR)blockloop 2> $(OBJDIR)intercept.tool.out | \
            tee $(OBJDIR)intercept.out & \
        until grep 'Signals are blocked' $(OBJDIR)intercept.out > /dev/null; \
            do sleep 5; \
            if ! kill -s 0 $$! > /dev/null 2>&1; then \
                echo "intercept.test: Aborted because Pin process terminated"; \
                exit 1; \
            fi; \
        done; \
        pid=`head -1 $(OBJDIR)intercept.out | sed 's/.*to pid//'`; \
        kill -HUP $$pid; \
        until grep 'Intercepting signal' $(OBJDIR)intercept.tool.out > /dev/null; \
            do sleep 1; \
            if ! kill -s 0 $$! > /dev/null 2>&1; then \
                echo "intercept.test: Aborted because Pin process terminated"; \
                exit 1; \
            fi; \
        done; \
        kill -HUP $$pid; \
        wait $$!
	$(PIN_CMP) $(OBJDIR)intercept.tool.out intercept.tool.reference
	rm $(OBJDIR)$(@:.test=.failed)

maskcheck.test: $(OBJDIR)maskcheck.tested $(OBJDIR)maskcheck.failed $(OBJDIR)maskcheck
	./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

sigflowcheck1.test: $(OBJDIR)sigflowcheck1.tested $(OBJDIR)sigflowcheck1.failed $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) $(OBJDIR)nestmask
	$(PIN) -t $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) -- ./$(OBJDIR)nestmask > $(OBJDIR)sigflowcheck1.out
	$(PIN_CMP) $(OBJDIR)sigflowcheck1.out sigflowcheck1.reference
	rm $(OBJDIR)$(@:.test=.failed)

sigflowcheck2.test: $(OBJDIR)sigflowcheck2.tested $(OBJDIR)sigflowcheck2.failed $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) $(OBJDIR)exithandler
	$(PIN) -t $(OBJDIR)sigflowcheck$(PINTOOL_SUFFIX) -- ./$(OBJDIR)exithandler > $(OBJDIR)sigflowcheck2.out
	$(PIN_CMP) $(OBJDIR)sigflowcheck2.out sigflowcheck2.reference
	rm $(OBJDIR)$(@:.test=.failed)

simpleasync.test: $(OBJDIR)simpleasync.tested $(OBJDIR)simpleasync.failed $(OBJDIR)simple $(OBJDIR)exittool$(PINTOOL_SUFFIX)
	$(PIN) -t $(OBJDIR)exittool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)simple > $(OBJDIR)simpleasync.out
	$(PIN_CMP) $(OBJDIR)simpleasync.out simpleasync.reference
	rm $(OBJDIR)$(@:.test=.failed)

simplesync.test: $(OBJDIR)segv $(OBJDIR)exittool$(PINTOOL_SUFFIX) $(OBJDIR)simplesync.tested $(OBJDIR)simplesync.failed
	$(PIN) -t $(OBJDIR)exittool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)segv > $(OBJDIR)simplesync.out
	$(PIN_CMP) $(OBJDIR)simplesync.out simplesync.reference
	rm $(OBJDIR)$(@:.test=.failed)

altstack.test: $(OBJDIR)altstack.tested $(OBJDIR)altstack.failed $(OBJDIR)altstack
	$(PIN) -- ./$(OBJDIR)altstack
	rm $(OBJDIR)$(@:.test=.failed)

faultcheck-ia32.test: $(OBJDIR)faultcheck-ia32.tested $(OBJDIR)faultcheck-ia32.failed $(OBJDIR)faultcheck-ia32
	./$(OBJDIR)faultcheck-ia32 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)faultcheck-ia32 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

faultcheck-intel64.test: $(OBJDIR)faultcheck-intel64.tested $(OBJDIR)faultcheck-intel64.failed $(OBJDIR)faultcheck-intel64
	./$(OBJDIR)faultcheck-intel64 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)faultcheck-intel64 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

faultcheck-ia64.test: $(OBJDIR)faultcheck-ia64.tested $(OBJDIR)faultcheck-ia64.failed $(OBJDIR)faultcheck-ia64
	./$(OBJDIR)faultcheck-ia64 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)faultcheck-ia64 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

faultcheck_flags_ia32_1.test: $(OBJDIR)faultcheck_flags_ia32_1.tested $(OBJDIR)faultcheck_flags_ia32_1.failed $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_tool1$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck_flags_ia32 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -t ./$(OBJDIR)faultcheck_flags_tool1$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck_flags_ia32 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

faultcheck_flags_ia32_2.test: $(OBJDIR)faultcheck_flags_ia32_2.tested $(OBJDIR)faultcheck_flags_ia32_2.failed $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_tool2$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck_flags_ia32 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -t ./$(OBJDIR)faultcheck_flags_tool2$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck_flags_ia32 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

faultcheck_flags_ia32_3.test: $(OBJDIR)faultcheck_flags_ia32_3.tested $(OBJDIR)faultcheck_flags_ia32_3.failed $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_ia32 $(OBJDIR)faultcheck_flags_tool3$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck_flags_ia32 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -t ./$(OBJDIR)faultcheck_flags_tool3$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck_flags_ia32 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

errcheck.test: $(OBJDIR)errcheck.tested $(OBJDIR)errcheck.failed $(OBJDIR)errcheck
	$(PIN) -- ./$(OBJDIR)errcheck
	rm $(OBJDIR)$(@:.test=.failed)

natcheck.test: $(OBJDIR)natcheck.tested $(OBJDIR)natcheck.failed $(OBJDIR)natcheck
	$(PIN) -- ./$(OBJDIR)natcheck
	rm $(OBJDIR)$(@:.test=.failed)

movsbtest_ia32.test: $(OBJDIR)movsbtest_ia32.tested $(OBJDIR)movsbtest_ia32.failed $(OBJDIR)movsbtest_ia32
	$(PIN) -- ./$(OBJDIR)movsbtest_ia32
	rm $(OBJDIR)$(@:.test=.failed)

movsbtest_ia32e.test: $(OBJDIR)movsbtest_ia32e.tested $(OBJDIR)movsbtest_ia32e.failed $(OBJDIR)movsbtest_ia32e
	$(PIN) -- ./$(OBJDIR)movsbtest_ia32e
	rm $(OBJDIR)$(@:.test=.failed)

##################################################################################################
# double, hlt, recursive, setcontext, sigaltstack, sigsuspend, and sigusr2 are only for Linux-ia32
##################################################################################################
double.test: $(OBJDIR)double.tested $(OBJDIR)double.failed $(OBJDIR)double
	./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

hlt.test: $(OBJDIR)hlt.tested $(OBJDIR)hlt.failed $(OBJDIR)hlt
	./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

recursive.test: $(OBJDIR)recursive.tested $(OBJDIR)recursive.failed $(OBJDIR)recursive
	./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

setcontext.test: $(OBJDIR)setcontext.tested $(OBJDIR)setcontext.failed $(OBJDIR)setcontext
	./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

sigaltstack.test: $(OBJDIR)sigaltstack.tested $(OBJDIR)sigaltstack.failed $(OBJDIR)sigaltstack
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(OBJDIR)$(@:.test=.failed)

sigsuspend.test: $(OBJDIR)sigsuspend.tested $(OBJDIR)sigsuspend.failed $(OBJDIR)sigsuspend
	$(PIN) -- ./$(OBJDIR)$(@:.test=)
	rm $(OBJDIR)$(@:.test=.failed)

sigusr2.test: $(OBJDIR)sigusr2.tested $(OBJDIR)sigusr2.failed $(OBJDIR)sigusr2
	./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)$(@:.test=) > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

fatallog.test: $(OBJDIR)fatallog.tested $(OBJDIR)fatallog.failed $(OBJDIR)fatal $(OBJDIR)logtool$(PINTOOL_SUFFIX)
	-$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o $(OBJDIR)fatallog.out -- ./$(OBJDIR)fatal; exit 0 
	$(PIN_CMP) $(OBJDIR)fatallog.out fatallog.reference
	rm $(OBJDIR)$(@:.test=.failed)

sigcont.test: $(OBJDIR)sigcont.tested $(OBJDIR)sigcont.failed $(OBJDIR)sigcont
	$(PIN) -- ./$(OBJDIR)sigcont
	rm $(OBJDIR)$(@:.test=.failed)

fastsigcheck.test: $(OBJDIR)fastsigchecktool$(PINTOOL_SUFFIX) $(OBJDIR)fpcheck $(OBJDIR)fastsigcheck.tested $(OBJDIR)fastsigcheck.failed
	$(PIN) -xyzzy -statistic -logfile $(OBJDIR)fastsigcheck.log -t $(OBJDIR)fastsigchecktool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)fpcheck
	#
	# This tests if the number of "slow" signals is less than 5%
	#
	pctSlow=`fgrep 'pin::signal::total/slow' $(OBJDIR)fastsigcheck.log | sed -e 's/.*norm: *\([0-9\.]*\).*/\1/'`; \
	    test `expr $$pctSlow \< 0.05` -eq "1"
	rm $(OBJDIR)$(@:.test=.failed)

sigenable.test: $(OBJDIR)sigenable$(PINTOOL_SUFFIX) $(OBJDIR)sigenableapp $(OBJDIR)sigenable.tested $(OBJDIR)sigenable.failed
	$(PIN) -t $(OBJDIR)sigenable$(PINTOOL_SUFFIX) -- ./$(OBJDIR)sigenableapp > $(OBJDIR)$(@:.test=.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(@:.test=.reference)
	rm $(OBJDIR)$(@:.test=.failed)

interceptall.test: $(OBJDIR)interceptall$(PINTOOL_SUFFIX) $(OBJDIR)interceptall.tested $(OBJDIR)interceptall.failed
	$(PIN) -t $(OBJDIR)interceptall$(PINTOOL_SUFFIX) -- $(TESTAPP) makefile $(OBJDIR)interceptall.makefile.copy
	rm $(OBJDIR)$(@:.test=.failed)

sigprocmask_short.test: $(OBJDIR)sigprocmask_short $(OBJDIR)sigprocmask_short.tested $(OBJDIR)sigprocmask_short.failed
	$(PIN) -- ./$(OBJDIR)sigprocmask_short
	rm $(OBJDIR)$(@:.test=.failed)

sig64.test: $(OBJDIR)sig64 $(OBJDIR)sig64.tested $(OBJDIR)sig64.failed
	$(PIN) -- ./$(OBJDIR)sig64
	rm $(OBJDIR)$(@:.test=.failed)

faulttool.test: $(OBJDIR)faulttool$(PINTOOL_SUFFIX) $(OBJDIR)simple $(OBJDIR)faulttool.tested $(OBJDIR)faulttool.failed
	@echo "The 'faulttool' test is expected to seg fault and dump core"
	-$(PIN) -t $(OBJDIR)faulttool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)simple > $(OBJDIR)$(@:.test=.out)
	grep "Tool (or Pin) caused signal" $(OBJDIR)$(@:.test=.out) > /dev/null
	grep "Location may be in in-lined analysis code, try re-running with '-inline 0' to debug it" $(OBJDIR)$(@:.test=.out) > /dev/null
	rm $(OBJDIR)$(@:.test=.failed)

dfcheck_ia32.test: $(OBJDIR)dfcheck_ia32.tested $(OBJDIR)dfcheck_ia32.failed $(OBJDIR)dfcheck_ia32
	$(PIN) -- ./$(OBJDIR)dfcheck_ia32
	rm $(OBJDIR)$(@:.test=.failed)

dfcheck_ia32e.test: $(OBJDIR)dfcheck_ia32e.tested $(OBJDIR)dfcheck_ia32e.failed $(OBJDIR)dfcheck_ia32e
	$(PIN) -- ./$(OBJDIR)dfcheck_ia32e
	rm $(OBJDIR)$(@:.test=.failed)

fatalsync.test: $(OBJDIR)fatalsync.tested $(OBJDIR)fatalsync.failed $(OBJDIR)fatalsync
	@echo "The 'fatalsync' test is expected to seg fault"
	-$(PIN) -- ./$(OBJDIR)fatalsync > $(OBJDIR)$(@:.test=.out)
	if grep "assertion failed" $(OBJDIR)$(@:.test=.out); then exit 1; fi
	rm $(OBJDIR)$(@:.test=.failed)

blockingsyscall.test: $(OBJDIR)blockingsyscall $(OBJDIR)blockingsyscall.tested $(OBJDIR)blockingsyscall.failed
	$(PIN) -- ./$(OBJDIR)blockingsyscall > $(OBJDIR)$(@:.test=.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(@:.test=.reference)
	rm $(OBJDIR)$(@:.test=.failed)

futex.test: $(OBJDIR)futex $(OBJDIR)futex.tested $(OBJDIR)futex.failed
	$(PIN) -- ./$(OBJDIR)futex
	rm $(OBJDIR)$(@:.test=.failed)

fptags_ia32.test: $(OBJDIR)fptags_ia32 $(OBJDIR)fptags_ia32.tested $(OBJDIR)fptags_ia32.failed
	$(PIN) -- ./$(OBJDIR)fptags_ia32
	rm $(OBJDIR)$(@:.test=.failed)

# currently have no way of distingushing between fpe signal generated by a real exception or
# sent by a user. this is a problem with the way FreeBSD kernel sets the si_code field in siginfo_t
# for now, we choose to support the synchronous mode and so the async test crashes.
#
asyncfpe.test: $(OBJDIR)asyncfpe $(OBJDIR)asyncfpe.tested $(OBJDIR)asyncfpe.failed
	$(PIN) -- ./$(OBJDIR)asyncfpe
	rm $(OBJDIR)$(@:.test=.failed)

segv_on_fetch.test: $(OBJDIR)segv_on_fetch $(OBJDIR)segv_on_fetch.tested $(OBJDIR)segv_on_fetch.failed
	$(PIN) -- ./$(OBJDIR)segv_on_fetch
	rm $(OBJDIR)$(@:.test=.failed)

fatal_block_sync.test: $(OBJDIR)fatal_block_sync $(OBJDIR)exittool$(PINTOOL_SUFFIX) $(OBJDIR)fatal_block_sync.tested $(OBJDIR)fatal_block_sync.failed
	@echo "The 'fatal_block_sync' test is expected to seg fault"
	-$(PIN) -t $(OBJDIR)exittool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)fatal_block_sync > $(OBJDIR)$(@:.test=.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(@:.test=.reference)
	rm $(OBJDIR)$(@:.test=.failed)

suspendmask.test: $(OBJDIR)suspendmask $(OBJDIR)suspendmask.tested $(OBJDIR)suspendmask.failed
	./$(OBJDIR)suspendmask > $(OBJDIR)$(@:.test=.out)
	$(PIN) -- ./$(OBJDIR)suspendmask > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

efault.test: $(OBJDIR)efault $(OBJDIR)efault.tested $(OBJDIR)efault.failed
	$(PIN) -- ./$(OBJDIR)efault
	rm $(OBJDIR)$(@:.test=.failed)

raise-exception-ia32.test: $(OBJDIR)raise-exception-ia32.tested $(OBJDIR)raise-exception-ia32.failed $(OBJDIR)faultcheck-ia32 $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck-ia32 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -t $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck-ia32 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)

raise-exception-intel64.test: $(OBJDIR)raise-exception-intel64.tested $(OBJDIR)raise-exception-intel64.failed $(OBJDIR)faultcheck-intel64 $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX)
	./$(OBJDIR)faultcheck-intel64 > $(OBJDIR)$(@:.test=.out)
	$(PIN) -t $(OBJDIR)raise-exception-tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)faultcheck-intel64 > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	rm $(OBJDIR)$(@:.test=.failed)


# we do not use the shortmask test on FreeBSD because it tests legacy behavior for linux system calls
# and this behavior is not applicable to FreeBSD. We currently do not intend to support old FreeBSD 
# signal syscalls etc. 
#
shortmask.test: $(OBJDIR)shortmask $(OBJDIR)shortmask.tested $(OBJDIR)shortmask.failed
	$(PIN) -- ./$(OBJDIR)shortmask
	rm $(OBJDIR)$(@:.test=.failed)

action-pending.test: $(OBJDIR)simple $(OBJDIR)action-pending$(PINTOOL_SUFFIX) $(OBJDIR)action-pending.tested $(OBJDIR)action-pending.failed
	$(PIN) -t $(OBJDIR)action-pending$(PINTOOL_SUFFIX) -- ./$(OBJDIR)simple
	rm $(OBJDIR)$(@:.test=.failed)

# Test the PIN_ReplayUnixSignal() API.  This test is disabled on IA64 because it uses IARG_CONTEXT,
# which isn't implemented there.
#
replay-signal.test: $(OBJDIR)replay-signal-app $(OBJDIR)replay-signal-tool$(PINTOOL_SUFFIX) $(OBJDIR)replay-signal.tested $(OBJDIR)replay-signal.failed
	$(PIN) -t $(OBJDIR)replay-signal-tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR)replay-signal-app > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.pin.out) $(@:.test=.reference)
	rm $(OBJDIR)$(@:.test=.failed)

# Test's Pin's handling of the SA_RESETHAND flag with a signal that is fatal by default.
#
resetsegv.test: $(OBJDIR)resetsegv $(OBJDIR)logtool$(PINTOOL_SUFFIX) $(OBJDIR)resetsegv.tested $(OBJDIR)resetsegv.failed
	-./$(OBJDIR)resetsegv > $(OBJDIR)$(@:.test=.out)
	-$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.tool.out) -- ./$(OBJDIR)resetsegv > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.tool.out) $(@:.test=.reference)
	rm $(OBJDIR)$(@:.test=.failed)

# Test's Pin's handling of the SA_RESETHAND flag with a signal that is ignored by default.
#
resetcont.test: $(OBJDIR)resetcont $(OBJDIR)logtool$(PINTOOL_SUFFIX) $(OBJDIR)resetcont.tested $(OBJDIR)resetcont.failed
	./$(OBJDIR)resetcont > $(OBJDIR)$(@:.test=.out)
	$(PIN) -t $(OBJDIR)logtool$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.tool.out) -- ./$(OBJDIR)resetcont > $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) $(OBJDIR)$(@:.test=.pin.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.tool.out) $(@:.test=.reference)
	rm $(OBJDIR)$(@:.test=.failed)

# Test that SafeCopy() works even if the application sets up a SEGV handler with SA_RESETHAND.
#
resetsegv-safecopy.test: $(OBJDIR)resetsegv $(OBJDIR)resetsegv-safecopy$(PINTOOL_SUFFIX) $(OBJDIR)resetsegv-safecopy.tested $(OBJDIR)resetsegv-safecopy.failed
	$(PIN) -t $(OBJDIR)resetsegv-safecopy$(PINTOOL_SUFFIX) -- ./$(OBJDIR)resetsegv
	rm $(OBJDIR)$(@:.test=.failed)

# Test the deprecated PIN_AddSignalInterceptFunction() function.
#
deprecated-intercept.test: $(OBJDIR)handle-usr1 $(OBJDIR)deprecated-intercept$(PINTOOL_SUFFIX) $(OBJDIR)deprecated-intercept.tested $(OBJDIR)deprecated-intercept.failed
	$(PIN) -t $(OBJDIR)deprecated-intercept$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.pass-handle.toolout) -p -- $(OBJDIR)handle-usr1 yes > $(OBJDIR)$(@:.test=.pass-handle.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.pass-handle.toolout) $(@:.test=.tool.reference)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.pass-handle.out) $(@:.test=.app-handle.reference)
	$(PIN) -t $(OBJDIR)deprecated-intercept$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.nopass-handle.toolout) -- $(OBJDIR)handle-usr1 yes > $(OBJDIR)$(@:.test=.nopass-handle.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.nopass-handle.toolout) $(@:.test=.tool.reference)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.nopass-handle.out) $(@:.test=.app-nohandle.reference)
	$(PIN) -t $(OBJDIR)deprecated-intercept$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.pass-nohandle.toolout) -p -- $(OBJDIR)handle-usr1 no > $(OBJDIR)$(@:.test=.pass-nohandle.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.pass-nohandle.toolout) $(@:.test=.tool.reference)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.pass-nohandle.out) $(@:.test=.app-nohandle.reference)
	$(PIN) -t $(OBJDIR)deprecated-intercept$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.nopass-nohandle.toolout) -- $(OBJDIR)handle-usr1 no > $(OBJDIR)$(@:.test=.nopass-nohandle.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.nopass-nohandle.toolout) $(@:.test=.tool.reference)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.nopass-nohandle.out) $(@:.test=.app-nohandle.reference)
	rm $(OBJDIR)$(@:.test=.failed)

# Test the behavior when a signal cannot be handled because the application's stack is bad.
#
# On Linux, the kernel delivers a SEGV, which also cannot be handled.  The kernel then
# sends a fatal SEGV.
# On BSD, the kernel converts the original signal into a fatal signal, which terminates the process.
#
# Note that older kernels have different behavior, but Pin emulates the behavior of the
# most recent kernel (as of Linux 2.6.30).
#
bad-stack-noalt.test: $(OBJDIR)bad-stack-$(TARGET_LONG) $(OBJDIR)reporter$(PINTOOL_SUFFIX) $(OBJDIR)bad-stack-noalt.tested $(OBJDIR)bad-stack-noalt.failed
	-$(PIN) -t $(OBJDIR)reporter$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.toolout) -- $(OBJDIR)bad-stack-$(TARGET_LONG) sigill noaltstack > $(OBJDIR)$(@:.test=.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) bad-stack-noalt-$(TARGET_OS_LONG).reference
	$(PIN_CMP) $(OBJDIR)$(@:.test=.toolout) bad-stack-noalt-$(TARGET_OS_LONG).toolreference
	rm $(OBJDIR)$(@:.test=.failed)

# Test the behavior when a signal cannot be handled because the application's stack is bad.
#
# On Linux, the kernel shoild deliver a SEGV, which the application handles on an alternate signal stack.
# On BSD, the kernel converts the original signal into a fatal signal, which terminates the process.
#
# Note that older kernels have different behavior, but Pin emulates the behavior of the
# most recent kernel (as of Linux 2.6.30).
#
bad-stack-alt.test: $(OBJDIR)bad-stack-$(TARGET_LONG) $(OBJDIR)reporter$(PINTOOL_SUFFIX) $(OBJDIR)bad-stack-alt.tested $(OBJDIR)bad-stack-alt.failed
	-$(PIN) -t $(OBJDIR)reporter$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.toolout) -- $(OBJDIR)bad-stack-$(TARGET_LONG) sigill altstack > $(OBJDIR)$(@:.test=.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) bad-stack-alt-$(TARGET_OS_LONG).reference
	$(PIN_CMP) $(OBJDIR)$(@:.test=.toolout) bad-stack-alt-$(TARGET_OS_LONG).toolreference
	rm $(OBJDIR)$(@:.test=.failed)

# Test the behavior when a sigretrun system call cannot read a signal context from the
# application's stack.
#
# On Linux, the kernel should send a SEGV, which the application handles on an alternate signal stack.
# On BSD, the kernel return EFAULT from the sigreturn system call.
#
# Note that older kernels have different behavior, but Pin emulates the behavior of the
# most recent kernel (as of Linux 2.6.30).
#
bad-stack-sigret.test: $(OBJDIR)bad-stack-$(TARGET_LONG) $(OBJDIR)reporter$(PINTOOL_SUFFIX) $(OBJDIR)bad-stack-sigret.tested $(OBJDIR)bad-stack-sigret.failed
	$(PIN) -t $(OBJDIR)reporter$(PINTOOL_SUFFIX) -o $(OBJDIR)$(@:.test=.toolout) -- $(OBJDIR)bad-stack-$(TARGET_LONG) sigreturn altstack > $(OBJDIR)$(@:.test=.out)
	$(PIN_CMP) $(OBJDIR)$(@:.test=.out) bad-stack-sigret-$(TARGET_OS_LONG).reference
	$(PIN_CMP) $(OBJDIR)$(@:.test=.toolout) bad-stack-sigret-$(TARGET_OS_LONG).toolreference
	rm $(OBJDIR)$(@:.test=.failed)

# Stress test for signal handling in a multi-threaded application.
#
mtstress.test: $(OBJDIR)mtstress $(OBJDIR)mtstress.tested $(OBJDIR)mtstress.failed
	$(PIN) -- $(OBJDIR)mtstress > $(OBJDIR)$(@:.test=.out)
	rm $(OBJDIR)$(@:.test=.failed)


dummy.test:


clean:
	rm -rf $(OBJDIR)
	rm -f pin.log core*
