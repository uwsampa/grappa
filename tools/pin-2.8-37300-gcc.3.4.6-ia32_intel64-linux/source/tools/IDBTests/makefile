##
## Tests that exercise the Intel debugger (IDB) with Pin's application-level
## debugging support.
##
## Note: These tests may not be run from the Pin kit unless you set the following
## variables on the "make" command line:
##
##   IDBDIR=<directory-containing-idb>
##   IDB=<path-to-idb>
##
## IDB is not distributed by the Pin team.
##

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

include ../makefile.$(TARGET_COMPILER).config

# This is a time limit (in seconds) we use when running the tests.  It's intentionally high
# to avoid timeouts when the system load is very high, which can happen in our nightly tests.
#
TLIMIT=600


APPS_x86_l   = simple thread-unix sleep mt-syscall-unix mt1
TOOLS_x86_lw = sleeptool usercmd-tool
TESTS_x86_l  = simple sleep thread-unix mtsyscall-unix-cont mtsyscall-unix-kill mt1 usercmd

# These tests can be enabled when we have an IDB that does not require the MS debug runtime
# libraries to be installed.
#
#TESTS_ia32_w    = simple-ia32-windows sleep-ia32-windows thread-ia32-windows mtsyscall-ia32-windows-cont \
#                  mtsyscall-ia32-windows-kill mt-terminate-process-syscall-ia32 mt-terminate-process-spin-ia32 \
#                  usercmd-ia32-windows
#TESTS_ia32e_w   = simple-intel64-windows sleep-intel64-windows thread-intel64-windows mtsyscall-intel64-windows-cont \
#                  mtsyscall-intel64-windows-kill mt-terminate-process-syscall-intel64 mt-terminate-process-spin-intel64 \
#                  usercmd-intel64-windows

# These can be enabled when the cygwin bug is fixed (see below).
#
#APPS_x86_w      = simple sleep mt-utility-windows
#TESTS_x86_w     = simple sleep thread-windows mtsyscall-windows-cont mtsyscall-windows-kill \
#                  mt-terminate-process-syscall mt-terminate-process-spin usercmd


apps_ia32_l   = $(APPS_x86) $(APPS_x86_l) $(APPS_x86_lw)
apps_ia32e_l  = $(APPS_x86) $(APPS_x86_l) $(APPS_x86_lw)
apps_ia32_w   = $(APPS_x86) $(APPS_x86_w) $(APPS_x86_lw)
apps_ia32e_w  = $(APPS_x86) $(APPS_x86_w) $(APPS_x86_lw)
tools_ia32_l  = $(TOOLS_x86) $(TOOLS_x86_l) $(TOOLS_x86_lw)
tools_ia32e_l = $(TOOLS_x86) $(TOOLS_x86_l) $(TOOLS_x86_lw)
tools_ia32_w  = $(TOOLS_x86) $(TOOLS_x86_w) $(TOOLS_x86_lw)
tools_ia32e_w = $(TOOLS_x86) $(TOOLS_x86_w) $(TOOLS_x86_lw)
tests_ia32_l  = $(TESTS_x86) $(TESTS_x86_l) $(TESTS_x86_lw)
tests_ia32e_l = $(TESTS_x86) $(TESTS_x86_l) $(TESTS_x86_lw)
tests_ia32_w  = $(TESTS_x86) $(TESTS_x86_w) $(TESTS_x86_lw)
tests_ia32e_w = $(TESTS_x86) $(TESTS_x86_w) $(TESTS_x86_lw)

apps  = $(APPS) $(APPS_$(TARGET)) $(APPS_$(TARGET_OS)) $(APPS_$(TARGET)_$(TARGET_OS)) $(apps_$(TARGET)_$(TARGET_OS))
tools = $(TOOLS) $(TOOLS_$(TARGET)) $(TOOLS_$(TARGET_OS)) $(TOOLS_$(TARGET)_$(TARGET_OS)) $(tools_$(TARGET)_$(TARGET_OS))
tests = $(TESTS) $(TESTS_$(TARGET)) $(TESTS_$(TARGET_OS)) $(TESTS_$(TARGET)_$(TARGET_OS)) $(tests_$(TARGET)_$(TARGET_OS)) dummy

# IDB is not supported on RedHat EL3 or SuSE 9.  Skip testing on those systems.
#
redhat = $(shell test -f /etc/redhat-release && cat /etc/redhat-release)
suse = $(shell test -f /etc/SuSE-release && cat /etc/SuSE-release)
ifeq ($(findstring AS release 3,$(redhat)),AS release 3)
  tests =
endif
ifeq ($(findstring VERSION = 9,$(suse)),VERSION = 9)
  tests =
endif


# The first two can be overridden on the command line.
#
IDBDIR = $(idb_local_dir)
IDB    = $(idb_local_dir)/idb$(EXEEXT)
idb_local_dir = idb-$(TARGET_OS_LONG)-$(TARGET_LONG)
idb_debug_flags1 = -log -logname $(OBJDIR)$(@:.test=.idblog)
idb_debug_flags2 = -tlo -xyzzy -mesgon log_debugger -logfile $(OBJDIR)$(@:.test=.pinlog) -unique_logfile --
idb_flags_l = -ts idb-tai-pin -tsf $(TAIPIN) -tco -pinbin $(PIN_NOFLAGS) -timeout $(TLIMIT) $(idb_debug_flags1) $(IDB_CONNECTION_FLAGS) -- $(idb_debug_flags2)
idb_flags_w = -ts idb-tai-pin -tsf $(TAIPIN) -tco -pinbin $(PIN_EXE) -timeout $(TLIMIT) $(idb_debug_flags1) $(IDB_CONNECTION_FLAGS) -- $(idb_debug_flags2)

# On Linux, IDB needs to find its own libraries, so add the IDB directory to LD_LIBRARY_PATH.
#
IDBCMD_l = LD_LIBRARY_PATH=$(IDBDIR) $(IDB) $(idb_flags_l)
IDBCMD_w = $(IDB) $(idb_flags_w)
IDBCMD   = $(IDBCMD_$(TARGET_OS))


all: $(apps:%=$(OBJDIR)%$(EXEEXT)) $(tools:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
test: $(tests:=.test)
tests-sanity: test

$(apps:%=$(OBJDIR)%$(EXEEXT)) $(tools:%=$(OBJDIR)%$(PINTOOL_SUFFIX)): $(OBJDIR)make-directory

$(OBJDIR)make-directory:
	mkdir -p $(OBJDIR)
	touch $(OBJDIR)make-directory


# If $(IDB) was not overridden, this will unpack the IDB kit from $(IDBKIT) into a
# local directory and we will use IDB from there.
#
$(tests:=.test): $(IDB)
$(IDB):
	if test -e $(IDBKIT); then \
	    mkdir -p $(idb_local_dir); \
	    tar -C $(idb_local_dir) -xzvf $(IDBKIT); \
	    chmod a+x $(idb_local_dir)/*; \
	fi


#
# Rules to build the applications
#
$(OBJDIR)simple$(EXEEXT): $(OBJDIR)simple.$(OBJEXT)
	$(CC) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTEXE)$@ $<
$(OBJDIR)simple.$(OBJEXT): $(OBJDIR)make-directory simple.c
	$(CC) $(DBG) $(COPT) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTOPT)$@ simple.c

$(OBJDIR)thread-unix$(EXEEXT): thread-unix.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTEXE)$@ $< $(APP_PTHREAD)

$(OBJDIR)mt-syscall-unix$(EXEEXT): mt-syscall-unix.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTEXE)$@ $< $(APP_PTHREAD)

$(OBJDIR)sleep$(EXEEXT): $(OBJDIR)sleep.$(OBJEXT)
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTEXE)$@ $<
$(OBJDIR)sleep.$(OBJEXT): $(OBJDIR)make-directory sleep.cpp
	$(CXX) $(DBG) $(COPT) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTOPT)$@ sleep.cpp

$(OBJDIR)mt-utility-windows$(EXEEXT): $(OBJDIR)mt-utility-windows.$(OBJEXT)
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTEXE)$@ $<
$(OBJDIR)mt-utility-windows.$(OBJEXT): $(OBJDIR)make-directory mt-utility-windows.cpp
	$(CXX) $(DBG) $(COPT) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTOPT)$@ mt-utility-windows.cpp

$(OBJDIR)mt1$(EXEEXT): mt1.cpp
	$(CXX) $(DBG) $(APP_CXXFLAGS) $(DBG_INFO_ALWAYS_IDB) $(OUTEXE)$@ $< $(APP_PTHREAD)


#
# Rules to build the tools
#
$(OBJDIR)sleeptool$(PINTOOL_SUFFIX): $(OBJDIR)sleeptool.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)
$(OBJDIR)sleeptool.$(OBJEXT): $(OBJDIR)make-directory sleeptool.cpp
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ sleeptool.cpp

$(OBJDIR)usercmd-tool$(PINTOOL_SUFFIX): $(OBJDIR)usercmd-tool.$(OBJEXT)
	$(PIN_LD) $(PIN_LDFLAGS) $(DBG) ${LINK_OUT}$@ $< $(PIN_LIBS)
$(OBJDIR)usercmd-tool.$(OBJEXT): $(OBJDIR)make-directory usercmd-tool.cpp
	$(CXX) $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ usercmd-tool.cpp


#
# Rules to run the tests.
#

# Simple test of the debugger.
#
simple.test: $(OBJDIR)simple$(EXEEXT) $(OBJDIR)simple.tested $(OBJDIR)simple.failed
	$(IDBCMD) -x simple.idb $(OBJDIR)simple$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can single-step over a system call.
#
sleep.test: $(OBJDIR)sleep$(EXEEXT) $(OBJDIR)sleeptool$(PINTOOL_SUFFIX) $(OBJDIR)sleep.tested $(OBJDIR)sleep.failed
	$(PIN) -t $(OBJDIR)sleeptool$(PINTOOL_SUFFIX) -o $(OBJDIR)sleep.idb -- $(OBJDIR)sleep$(EXEEXT)
	$(IDBCMD) -x $(OBJDIR)sleep.idb $(OBJDIR)sleep$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Simple tests of a threaded program.
#
thread-unix.test: $(OBJDIR)thread-unix$(EXEEXT) $(OBJDIR)thread-unix.tested $(OBJDIR)thread-unix.failed
	$(IDBCMD) -x thread.idb $(OBJDIR)thread-unix$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

thread-windows.test: $(OBJDIR)mt-utility-windows$(EXEEXT) $(OBJDIR)thread-windows.tested $(OBJDIR)thread-windows.failed
	$(IDBCMD) -x thread.idb $(OBJDIR)mt-utility-windows$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can continue an MT program while one thread is blocked in a system call.
#
mtsyscall-windows-cont.test: $(OBJDIR)mt-utility-windows$(EXEEXT) $(OBJDIR)mtsyscall-windows-cont.tested $(OBJDIR)mtsyscall-windows-cont.failed
	$(IDBCMD) -x mtsyscall-windows-cont.idb $(OBJDIR)mt-utility-windows$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

mtsyscall-unix-cont.test: $(OBJDIR)mt-syscall-unix$(EXEEXT) $(OBJDIR)mtsyscall-unix-cont.tested $(OBJDIR)mtsyscall-unix-cont.failed
	$(IDBCMD) -x mtsyscall-unix-cont.idb $(OBJDIR)mt-syscall-unix$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can kill an MT program while one thread is blocked in a system call.
#
mtsyscall-windows-kill.test: $(OBJDIR)mt-utility-windows$(EXEEXT) $(OBJDIR)mtsyscall-windows-kill.tested $(OBJDIR)mtsyscall-windows-kill.failed
	$(IDBCMD) -x mtsyscall-windows-kill.idb $(OBJDIR)mt-utility-windows$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

mtsyscall-unix-kill.test: $(OBJDIR)mt-syscall-unix$(EXEEXT) $(OBJDIR)mtsyscall-unix-kill.tested $(OBJDIR)mtsyscall-unix-kill.failed
	$(IDBCMD) -x mtsyscall-unix-kill.idb $(OBJDIR)mt-syscall-unix$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can debug a Windows MT application that kills itself via TerminateProcess()
# while the other thread is blocked in a system call.
#
mt-terminate-process-syscall.test: $(OBJDIR)mt-utility-windows$(EXEEXT) $(OBJDIR)mt-terminate-process-syscall.tested $(OBJDIR)mt-terminate-process-syscall.failed
	$(IDBCMD) -x $(@:.test=.idb) $(OBJDIR)mt-utility-windows$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mt-terminate-process.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test that we can debug a Windows MT application that kills itself via TerminateProcess()
# while the other thread is in a spin loop.
#
mt-terminate-process-spin.test: $(OBJDIR)mt-utility-windows$(EXEEXT) $(OBJDIR)mt-terminate-process-spin.tested $(OBJDIR)mt-terminate-process-spin.failed
	$(IDBCMD) -x $(@:.test=.idb) $(OBJDIR)mt-utility-windows$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mt-terminate-process.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# This is a test case from Peter that demonstrated several bugs.
#
mt1.test: $(OBJDIR)mt1$(EXEEXT) $(OBJDIR)mt1.tested $(OBJDIR)mt1.failed
	$(IDBCMD) -x $(@:.test=.idb) $(OBJDIR)mt1$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mt1.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

# Test of the user-command feature in IDB.
#
usercmd.test: $(OBJDIR)simple$(EXEEXT) $(OBJDIR)usercmd-tool$(PINTOOL_SUFFIX) $(OBJDIR)usercmd.tested $(OBJDIR)usercmd.failed
	$(IDBCMD) -tlo -t $(OBJDIR)usercmd-tool$(PINTOOL_SUFFIX) -- -x $(@:.test=.idb) $(OBJDIR)simple$(EXEEXT) > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p $(@:.test=.compare) -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)


# A bug in cygwin prevents us from being able to compile with $(DBG_INFO_ALWAYS_IDB)
# on Windows when run from our automated scripts.  As a workaround, these tests
# are pre-compiled and committed to the source tree.  If that cygwin bug is fixed,
# they can be removed and we can use the tests above.  The cygwin bug is described here:
#
#   http://cygwin.com/ml/cygwin/2008-08/msg00157.html
#
# These tests are disabled for now because we do not have an IDB that is linked with
# the release version of runtime libraries.  The test systems don't have the
# debug versions of the libraries installed, so our IDB won't run there.
#
simple-$(TARGET_LONG)-windows.test: simple-$(TARGET_LONG)-windows.exe $(OBJDIR)make-directory $(OBJDIR)simple-$(TARGET_LONG)-windows.tested $(OBJDIR)simple-$(TARGET_LONG)-windows.failed
	$(IDBCMD) -x simple.idb simple-$(TARGET_LONG)-windows.exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p simple.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

sleep-$(TARGET_LONG)-windows.test: sleep-$(TARGET_LONG)-windows.exe $(OBJDIR)sleeptool$(PINTOOL_SUFFIX) $(OBJDIR)make-directory $(OBJDIR)sleep-$(TARGET_LONG)-windows.tested $(OBJDIR)sleep-$(TARGET_LONG)-windows.failed
	$(PIN) -t $(OBJDIR)sleeptool$(PINTOOL_SUFFIX) -o $(OBJDIR)sleep.idb -- sleep-$(TARGET_LONG)-windows.exe
	$(IDBCMD) -x $(OBJDIR)sleep.idb sleep-$(TARGET_LONG)-windows.exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p sleep.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

thread-$(TARGET_LONG)-windows.test: mt-utility-windows-$(TARGET_LONG).exe $(OBJDIR)make-directory $(OBJDIR)thread-$(TARGET_LONG)-windows.tested $(OBJDIR)thread-$(TARGET_LONG)-windows.failed
	$(IDBCMD) -x thread.idb mt-utility-windows-$(TARGET_LONG).exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p thread-windows.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

mtsyscall-$(TARGET_LONG)-windows-cont.test: mt-utility-windows-$(TARGET_LONG).exe $(OBJDIR)make-directory $(OBJDIR)mtsyscall-$(TARGET_LONG)-windows-cont.tested $(OBJDIR)mtsyscall-$(TARGET_LONG)-windows-cont.failed
	$(IDBCMD) -x mtsyscall-windows-cont.idb mt-utility-windows-$(TARGET_LONG).exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mtsyscall-windows-cont.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

mtsyscall-$(TARGET_LONG)-windows-kill.test: mt-utility-windows-$(TARGET_LONG).exe $(OBJDIR)make-directory $(OBJDIR)mtsyscall-$(TARGET_LONG)-windows-kill.tested $(OBJDIR)mtsyscall-$(TARGET_LONG)-windows-kill.failed
	$(IDBCMD) -x mtsyscall-windows-kill.idb mt-utility-windows-$(TARGET_LONG).exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mtsyscall-windows-kill.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

mt-terminate-process-syscall-$(TARGET_LONG).test: mt-utility-windows-$(TARGET_LONG).exe $(OBJDIR)make-directory $(OBJDIR)mt-terminate-process-syscall-$(TARGET_LONG).tested $(OBJDIR)mt-terminate-process-syscall-$(TARGET_LONG).failed
	$(IDBCMD) -x mt-terminate-process-syscall.idb mt-utility-windows-$(TARGET_LONG).exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mt-terminate-process.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

mt-terminate-process-spin-$(TARGET_LONG).test: mt-utility-windows-$(TARGET_LONG).exe $(OBJDIR)make-directory $(OBJDIR)mt-terminate-process-spin-$(TARGET_LONG).tested $(OBJDIR)mt-terminate-process-spin-$(TARGET_LONG).failed
	$(IDBCMD) -x mt-terminate-process-spin.idb mt-utility-windows-$(TARGET_LONG).exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p mt-terminate-process.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)

usercmd-$(TARGET_LONG)-windows.test: simple-$(TARGET_LONG)-windows.exe $(OBJDIR)usercmd-tool$(PINTOOL_SUFFIX) $(OBJDIR)make-directory $(OBJDIR)usercmd-$(TARGET_LONG)-windows.tested $(OBJDIR)usercmd-$(TARGET_LONG)-windows.failed
	$(IDBCMD) -tlo -t $(OBJDIR)usercmd-tool$(PINTOOL_SUFFIX) -- -x usercmd.idb simple-$(TARGET_LONG)-windows.exe > $(OBJDIR)$(@:.test=.idbout)
	$(PYTHON) ../compare.py -p usercmd.compare -c $(OBJDIR)$(@:.test=.idbout)
	rm -f $(OBJDIR)$(@:.test=.failed)


dummy.test:


clean:
	rm -rf $(OBJDIR)
	rm -rf $(idb_local_dir)
	rm -rf pin.log
