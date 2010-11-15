
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
SUF = $(PINTOOL_SUFFIX)

SANITY_TOOLS = 
TOOLS =
APPS =

# rc added this test, but not ready for general testing
#TOOL_ROOTS = replaceebx


# linux tests go here.  if PROBE=0 is specified on the command line, do not run probe tests.
ifeq ($(PROBE),1)
 ifneq ($(ENABLE_VS),1)
  ifeq ($(TARGET_OS),l)

    # probe2 & probe4 are negative tests (they are supposed to fail)
    ifeq ($(TARGET),ia32)
      TOOL_ROOTS += probemalloctrace iprtool malloctrace2 symbolnames replace_exit 
      TOOL_ROOTS += proto1 proto2 proto3 proto4 replace_empty replacesig_empty
      TOOL_ROOTS +=  probe1 probe2 probe3 probe5 context_probe malloctrace_locktest
      TOOL_ROOTS += probe_on_probe1
      TOOL_ROOTS += init remove_probe debugtest probe8 probesleep probe10
      TOOL_ROOTS += probe_err1 probe_err2 probe_err4 probe_err5 probe_err6 remove_probe2
      TOOL_ROOTS += insert1 insert2 insert3 insert4 insert5 insert6 insert7 insert8 insert9
      TOOL_ROOTS += insert10 insert11  insert12 code_range
      TOOL_ROOTS += signals spinlock_replsig spinlock_repl spinlock_insert
      TOOL_ROOTS += sempost_repl svcraw_repl exit_repl relocate_rtn
      TOOL_ROOTS += load_map insert_call_probed error-lin-probe error-lin-jit
      REPLACEAPP = $(OBJDIR)replaceapp_lin32
      REPLACECALL = $(OBJDIR)replacecall_lin32
      ERRCALL = $(OBJDIR)err_call_lin32
      REPLACEFUN = $(OBJDIR)replacefun_lin32
      ERRREPLACE = $(OBJDIR)err_replace_lin32
      BADJUMP= $(OBJDIR)bad_jump_lin32
      GOODJUMP= $(OBJDIR)good_jump_lin32
      FALL_THRU = $(OBJDIR)fall_thru_lin32
      BAD_CALL = $(OBJDIR)bad_call_lin32
      HIGH_TARGET = $(OBJDIR)high_target_lin32
      APP_TRACE = $(OBJDIR)app_trace
      APPS += replaceapp_lin32 replacecall_lin32 err_call_lin32 replacefun_lin32 err_replace_lin32 
      APPS += bad_jump_lin32 good_jump_lin32 fall_thru_lin32 bad_call_lin32 high_target_lin32
      APPS += app_trace protofoo simplefoo thd_malloc signals_app spin_lock_app sempost_app svcraw_app
      APPS += exit_app relocate_app insert_call_probed_app load_map_app
    endif

    ifeq ($(TARGET),ia32e)
      TOOL_ROOTS += probemalloctrace iprtool malloctrace2 symbolnames replace_empty replacesig_empty 
      TOOL_ROOTS += proto1 proto2 proto3 proto4 replace_exit
      TOOL_ROOTS += probe1 probe2 probe3 probe4 probe5 probe6 probe7 probe8 context_probe 
      TOOL_ROOTS += probe_on_probe1 syscall_in_probe
      TOOL_ROOTS += init remove_probe debugtest probesleep probe9 probe10
      TOOL_ROOTS += malloctrace_locktest probe_err1 probe_err2 probe_movip probe_err4 probe_err5
      TOOL_ROOTS += remove_probe2 probe_err6
      TOOL_ROOTS += insert1 insert2 insert3 insert4 insert5 insert6 insert7 insert8 insert9
      TOOL_ROOTS += insert10 insert11 insert12 code_range 
      TOOL_ROOTS += spinlock_replsig spinlock_repl spinlock_insert
      TOOL_ROOTS += sempost_repl svcraw_repl exit_repl relocate_rtn
      TOOL_ROOTS += load_map insert_call_probed error-lin-probe error-lin-jit
      REPLACEAPP = $(OBJDIR)replaceapp_lin64
      REPLACECALL = $(OBJDIR)replacecall_lin64
      REPLACEDISP = $(OBJDIR)replacedisp_lin64
      REPLACEJMP = $(OBJDIR)replacejmp_lin64
      REPLACESHORTY = $(OBJDIR)replaceshorty_lin64
      ERRREPLACE = $(OBJDIR)err_replace_lin64
      ERRCALL = $(OBJDIR)err_call_lin64
      REPLACEFUN = $(OBJDIR)replacefun_lin64
      BADJUMP= $(OBJDIR)bad_jump_lin64
      GOODJUMP= $(OBJDIR)good_jump_lin64
      MOVE_IP = $(OBJDIR)move_ip_lin64
      FALL_THRU = $(OBJDIR)fall_thru_lin64
      BAD_CALL = $(OBJDIR)bad_call_lin64
      HIGH_TARGET = $(OBJDIR)high_target_lin64
      APP_TRACE = $(OBJDIR)app_trace
      SYSPROBEAPP = $(OBJDIR)syscall_in_probe_app
      
      APPS += replaceapp_lin64 replacecall_lin64 replacedisp_lin64 replacejmp_lin64 replaceshorty_lin64 
      APPS += err_replace_lin64 replacefun_lin64 err_call_lin64 bad_jump_lin64 good_jump_lin64
      APPS += move_ip_lin64 fall_thru_lin64 bad_call_lin64 high_target_lin64 app_trace syscall_in_probe_app
      APPS += protofoo simplefoo thd_malloc spin_lock_app sempost_app svcraw_app exit_app relocate_app
      APPS += insert_call_probed_app load_map_app
    endif

    ifeq ($(TARGET),ipf)
      TOOL_ROOTS += probemalloctrace iprtool proto1 proto2
      TOOL_ROOTS += replace_empty replace_empty_twice replacesig_empty malloctrace_tpbug 
      TOOL_ROOTS += remove_probe remove_probe2 probesleep
      TOOL_ROOTS += insert1 insert2 insert3 insert4
    endif

    TOOLS += $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
    CP = $(TESTAPP)
    DLTEST = $(OBJDIR)dltest
    DLTEST_TP = $(OBJDIR)dltest-tp
    MALLOCWRAPPERS = $(OBJDIR)mallocwrappers.so
    IPR = $(OBJDIR)ipr
    UNLOADTEST = $(OBJDIR)unloadtest
    LITTLE_MALLOC=$(OBJDIR)little_malloc
    SLEEP = $(OBJDIR)sleep
    SIMPLEFOO9 = $(OBJDIR)simplefoo9
    SIMPLEFOO10 = $(OBJDIR)simplefoo10
    APP_TRACE = $(OBJDIR)app_trace
    ERROR_APP = $(OBJDIR)error-app
    APPS += dltest dltest-tp ipr unloadtest little_malloc sleep simplefoo9 simplefoo10 app_trace error-app mallocwrappers.so
    APPS += empty
  endif
 endif # Enable VS
endif # PROBE

# windows tests go here:
ifeq ($(TARGET_OS),w)
  # proto2 uses a local symbol.  Only imports and exports can be 
  # replaced on windows.
  TOOL_ROOTS += iprtool proto1win proto3win probemalloctrace proto1win-err
  TOOL_ROOTS += replace_empty replacesig_empty probecdecl tpss
  TOOL_ROOTS += insert1win insert2win insert3win insert10 insert11
  TOOL_ROOTS += insert12 code_range error-win-probe error-win-jit
  SIMPLEFOO9 = $(OBJDIR)winfoo9.exe
  SIMPLEFOO10 = $(OBJDIR)winfoo10.exe
  ERROR_APP = $(OBJDIR)error-app.exe
 
  # unresolved external symbol ___intel_new_proc_init when using icl
  ifneq ($(CC),icl)
    TOOL_ROOTS += probesleep
  endif
  TOOL_ROOTS += probe_on_probe1 probe_on_probe2

  ifeq ($(TARGET_COMPILER),ms)
    TOOL_ROOTS += exception_in_probe exception_in_probe_on_probe exception_in_probed_call
    TOOL_ROOTS += exception_in_probe_sig exception_in_probe_on_probe_sig exception_in_probed_call_sig
    TOOL_ROOTS += exception_in_probed_call_after
    TOOL_ROOTS += malloctrace2win ssewin
    DLTEST=$(OBJDIR)dltestwin.exe
    MALLOCWRAPPERS=$(OBJDIR)mallocwrapperswin.dll
    IPR = $(OBJDIR)ipr.exe
    UNLOADTEST=$(OBJDIR)unloadtestwin.exe
    LITTLE_MALLOC=$(OBJDIR)little_malloc.exe
    SLEEP = $(OBJDIR)sleep.exe
    APP_TRACE = $(OBJDIR)app_trace.exe

    ifeq ($(TARGET),ia32)
      TOOL_ROOTS += probestdcall probefastcall probeheapalloc shortBB
      BBTEST=$(OBJDIR)shortBBapp.exe
    endif

    ifeq ($(TARGET),ia32e)
      TOOL_ROOTS += shortBB64
      BBTEST=$(OBJDIR)shortBB64app.exe
    endif

  else
    DLTEST=$(OBJDIR)dltest.exe
    MALLOCWRAPPERS=$(OBJDIR)mallocwrappers.dll
    IPR = ipr
  endif

  TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
  CP = $(TESTAPP)
  REPLACEAPP = $(OBJDIR)replaceapp.exe
  CREATE_PROCESS_APP = $(OBJDIR)create_process_app.exe

endif

# mac tests go here:
ifeq ($(TARGET_OS),m)
  #TOOL_ROOTS = malloctrace2 probemalloctrace
  TOOL_ROOTS = 

  TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
  DLTEST=$(OBJDIR)dltest-mac
  MALLOCWRAPPERS=$(OBJDIR)mallocwrappers-mac.so
endif

# FreeBSD tests go here.  if PROBE=0 is specified on the command line, do not run probe tests.
ifeq ($(PROBE),1)
  ifneq ($(ENABLE_VS),1)
    ifeq ($(TARGET_OS),b)
       # probe2 & probe4 are negative tests (they are supposed to fail)
      ifeq ($(TARGET),ia32e)
        #FIXME: malloctrace2 is removed from FreeBSD because of dependency on libdl
        TOOL_ROOTS += probemalloctrace iprtool symbolnames replace_empty replacesig_empty
        TOOL_ROOTS += proto1 proto2 proto3 proto4 replace_exit
        TOOL_ROOTS += probe1 probe2 probe3 probe4 probe5 probe6 probe7 probe8 context_probe
        #FIXME: init is removed from FreeBSD because of different _init prototype and no libpthread
        TOOL_ROOTS += remove_probe debugtest probesleep probe9
        TOOL_ROOTS += malloctrace_locktest probe_err1 probe_err2 probe_movip probe_err4 probe_err5
        TOOL_ROOTS += probe_err6
        TOOL_ROOTS += insert1 insert2 insert3 insert4 insert5 insert6 insert7 insert8 insert9
        TOOL_ROOTS += insert10 insert11
        REPLACEAPP = $(OBJDIR)replaceapp_lin64
        REPLACECALL = $(OBJDIR)replacecall_lin64
        REPLACEDISP = $(OBJDIR)replacedisp_lin64
        REPLACEJMP = $(OBJDIR)replacejmp_lin64
        REPLACESHORTY = $(OBJDIR)replaceshorty_lin64
        ERRREPLACE = $(OBJDIR)err_replace_lin64
        ERRCALL = $(OBJDIR)err_call_lin64
        REPLACEFUN = $(OBJDIR)replacefun_lin64
        BADJUMP= $(OBJDIR)bad_jump_lin64
        MOVE_IP = $(OBJDIR)move_ip_lin64
        FALL_THRU = $(OBJDIR)fall_thru_lin64
        BAD_CALL = $(OBJDIR)bad_call_lin64
        HIGH_TARGET = $(OBJDIR)high_target_lin64
        APP_TRACE = $(OBJDIR)app_trace
        ERROR_APP = $(OBJDIR)error-app
        APPS += replaceapp_lin64 replacecall_lin64 replacedisp_lin64 replacejmp_lin64
        APPS += err_replace_lin64 err_call_lin64 replacefun_lin64 bad_jump_lin64 move_ip_lin64
        APPS += fall_thru_lin64 bad_call_lin64  high_target_lin64 app_trace error-app simplefoo
        APPS += thd_malloc
      endif

      TOOLS += $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
      CP = $(TESTAPP)
      DLTEST = $(OBJDIR)dltest
      DLTEST_TP = $(OBJDIR)dltest-tp
      MALLOCWRAPPERS = $(OBJDIR)mallocwrappers.so
      IPR = $(OBJDIR)ipr
      UNLOADTEST = $(OBJDIR)unloadtest
      LITTLE_MALLOC=$(OBJDIR)little_malloc
      SLEEP = $(OBJDIR)sleep
      SIMPLEFOO9 = $(OBJDIR)simplefoo9
      SIMPLEFOO10 = $(OBJDIR)simplefoo10
      
      APPS += dltest dltest-tp ipr unloadtest little_malloc sleep simplefoo9 simplefoo10 mallocwrappers.so

    endif
  endif # Enable VS
endif # PROBE

# On FreeBSD, libdl is part of libc, and not a separate lib
ifeq ($(TARGET_OS),b)
  DL_LIB = 
else
  DL_LIB = -ldl
endif

TESTS_TO_RUN = $(TOOL_ROOTS:%=%.test)

ifeq ($(PROBE),1)
  ifeq ($(TARGET_OS),l)
     ifeq ($(TARGET),ia32e)
        TESTS_TO_RUN += insert1_no_xmm.test
     endif
     ifeq ($(TARGET),ia32)
        TESTS_TO_RUN += insert1_no_xmm.test
     endif
  endif
endif

SANITY_TESTS = $(TESTS_TO_RUN)


APPS_BINARY_FILES = $(APPS:%=$(OBJDIR)%)

ifeq ($(TARGET_OS),w)
all: tools 
else
all: tools $(APPS_BINARY_FILES)
endif

tools: $(OBJDIR) $(TOOLS)
test: $(OBJDIR) $(TESTS_TO_RUN)
tests-sanity: $(OBJDIR) $(SANITY_TESTS)

$(OBJDIR):
	mkdir -p $(OBJDIR)


###########################################
#                                         #
#    Applications                         #
#                                         #
###########################################

$(OBJDIR)traceapp: traceapp.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I../../include -I. -o $@ traceapp.c -lpinapp -g

$(OBJDIR)replaceapp_lin32: replaceapp.c $(OBJDIR)do_nothing_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)replacecall_lin32: replacecall.c $(OBJDIR)do_nothing_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)replacefun_lin32: replacefun.c $(OBJDIR)do_nothing_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)do_nothing_lin32.o: do_nothing_lin32.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c -o $@ $<

$(OBJDIR)replaceapp_lin64: replaceapp.c $(OBJDIR)do_nothing_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)replacecall_lin64: replacecall.c $(OBJDIR)do_nothing_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)replacefun_lin64: replacefun.c $(OBJDIR)do_nothing_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)replacejmp_lin64: replacejmp.c $(OBJDIR)pc64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@ -g

$(OBJDIR)replaceshorty_lin64: replaceshorty.c $(OBJDIR)do_nothing_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@ -g

$(OBJDIR)replacedisp_lin64: replacedisp.c $(OBJDIR)pc64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@ -g

$(OBJDIR)pc64.o: pc64.s 
	$(CC) $(APP_CXXFLAGS)  -x assembler-with-cpp -c $< -o $@

$(OBJDIR)do_nothing_lin64.o: do_nothing_lin64.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)getesp_ms.obj: getesp_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)probestdcall_app: probestdcall_app.c  $(OBJDIR)getesp_ms.obj  probe_stdcall_fastcall.h
	cp probestdcall_app.exe.zi $(OBJDIR)probestdcall_app.exe
	cp probestdcall_app.pdb.zi $(OBJDIR)probestdcall_app.pdb

$(OBJDIR)probefastcall_app: probefastcall_app.c  $(OBJDIR)getesp_ms.obj probe_stdcall_fastcall.h
	cp probefastcall_app.exe.zi $@.exe
	cp probefastcall_app.pdb.zi $@.pdb

$(OBJDIR)bad_jump_lin32: bad_jump.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)err_replace_lin32: err_replace.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)err_call_lin32: err_call.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)local_branch_lin32.o: local_branch_lin32.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c -o $@ $<

$(OBJDIR)good_jump_lin32: good_jump.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)fall_thru_lin32: fall_thru.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)move_ip_lin32: move_ip.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)bad_call_lin32: bad_call.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)bad_jump_lin64: bad_jump.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)high_target_lin32: high_target.c $(OBJDIR)local_branch_lin32.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)err_replace_lin64: err_replace.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)err_call_lin64: err_call.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)local_branch_lin64.o:local_branch_lin64.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)good_jump_lin64: good_jump.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)fall_thru_lin64: fall_thru.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)move_ip_lin64: move_ip.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)bad_call_lin64: bad_call.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)high_target_lin64: high_target.c $(OBJDIR)local_branch_lin64.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $+ -o $@  -g

$(OBJDIR)protofoo: protofoo.c protobar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ protofoo.c protobar.c -g

$(OBJDIR)protorfoo: protofoo.c protorbar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ protofoo.c protorbar.c -g

$(OBJDIR)simplefoo10: simplefoo10.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ simplefoo10.c simplebar.c -g

$(OBJDIR)winfoo10.exe: simplefoo10.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo10.c simplebar.c

$(OBJDIR)winfast.exe: simplefast10.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefast10.c simplebar.c

$(OBJDIR)winstd.exe: simplestd10.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplestd10.c simplebar.c

$(OBJDIR)simplefoo9: simplefoo9.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ simplefoo9.c simplebar.c -g

$(OBJDIR)winfoo9.exe: simplefoo9.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo9.c simplebar.c

$(OBJDIR)empty: empty.c emptysub.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. $(OUTEXE)$@ empty.c emptysub.c -O3

$(OBJDIR)load_map_app: load_map_app.cpp $(OBJDIR)libone.so
	$(CXX) $(APP_CXXFLAGS) -g ${OUTEXE}$@ load_map_app.cpp -L  $(OBJDIR) -lone
	
$(OBJDIR)libone.so: one.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)libone.so -fpic -shared one.c -g


$(OBJDIR)little_malloc: little_malloc.c
	$(CC) $(APP_CXXFLAGS) -g little_malloc.c -o $@

$(OBJDIR)little_malloc.exe: little_malloc.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ little_malloc.c

$(OBJDIR)ipr: ipr.c
	$(CC) $(APP_CXXFLAGS) -g -o $@ ipr.c

$(OBJDIR)ipr.exe: ipr.c
	$(CC) ipr.c $(OUTEXE)$@

$(OBJDIR)sleep: sleep.c
	$(CC) $(APP_CXXFLAGS) -g -o $@ sleep.c

$(OBJDIR)sleep.exe: sleepwin.c
	$(CC) sleepwin.c $(OUTEXE)$@

$(OBJDIR)mallocwrappers.so: mallocwrappers.c
	$(CC) $(APP_CXXFLAGS) -o $@ -fpic -shared mallocwrappers.c -g

$(OBJDIR)mallocwrapperswin.dll: mallocwrapperswin.c
	$(CC) mallocwrapperswin.c /nologo $(PIN_CXXFLAGS) /link /dll /out:$@

$(OBJDIR)mallocwrappers.dll: mallocwrapperswin.c
	$(CC) -o $(OBJDIR)mallocwrapperswin.dll -shared mallocwrapperswin.c -g  $(APP_CXXFLAGS)

$(OBJDIR)mallocwrappers-mac.so: mallocwrappers.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)mallocwrappers-mac.so -fPIC -dynamiclib mallocwrappers.c -g

$(OBJDIR)dltest: dltest.c one.c two.c 
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)two.so -fpic -shared two.c -g
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)one.so -fpic -shared one.c -g
	$(CC) -o $(OBJDIR)dltest $(APP_CXXFLAGS) dltest.c $(DL_LIB) -Wl,-rpath,`pwd`\/$(OBJDIR) -g

$(OBJDIR)dltest-tp: dltest.c one.c two.c 
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)two.so -fpic -shared two.c -g
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)one.so -fpic -shared one.c -g
	$(CC) -o $(OBJDIR)dltest-tp $(APP_CXXFLAGS) dltest.c $(DL_LIB) -Wl,-rpath,`pwd`\/$(OBJDIR) -g $(APP_PTHREAD)

$(OBJDIR)dltest-pf: dltestpf.c one.c two.c 
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)two.so -fpic -shared two.c -g
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)one.so -fpic -shared one.c -g
	$(CC) -o $(OBJDIR)dltest-pf $(APP_CXXFLAGS) dltestpf.c $(DL_LIB) -Wl,-rpath,`pwd`\/$(OBJDIR) -g

$(OBJDIR)dltest-mac: dltest.c one.c two.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)two.so -fPIC -dynamiclib two.c -g
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)one.so -fPIC -dynamiclib one.c -g
	$(CC) -o $(OBJDIR)dltest-mac $(APP_CXXFLAGS) dltest.c $(DL_LIB) -g


$(OBJDIR)dltest.exe: dltestwin.c onewin.c twowin.c 
	$(CC) -o $(OBJDIR)two.dll -shared twowin.c $(APP_CXXFLAGS)
	$(CC) -o $(OBJDIR)one.dll -shared onewin.c $(APP_CXXFLAGS)
	$(CC) -o $(OBJDIR)dltest.exe -g dltestwin.c $(APP_CXXFLAGS) 

# build with -O0 to prevent inlining
$(OBJDIR)app_trace: app_trace.c
	$(CC) -o $(OBJDIR)app_trace $(APP_CXXFLAGS) -O0 -g app_trace.c

# build with -/Od to prevent inlining
$(OBJDIR)app_trace.exe: app_trace.c
	$(CC) /Fe$@ app_trace.c /Od /nologo $(APP_CXXFLAGS) 

$(OBJDIR)shortBBapp.exe: shortBBapp.c 
	$(CC) -o $@ $(APP_CXXFLAGS) shortBBapp.c

$(OBJDIR)shortBB64app.exe: shortBB64app.c $(OBJDIR)shortBB64foo.obj
	$(CC) $(OUTEXE)$@ $(APP_CXXFLAGS) $+

$(OBJDIR)shortBB64foo.obj: shortBB64foo.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)dltestwin.exe: dltestwin.c onewin.c twowin.c 
	$(CC) twowin.c  /nologo $(APP_CXXFLAGS) /link /dll /out:$(OBJDIR)two.dll
	$(CC) onewin.c  /nologo $(APP_CXXFLAGS) /link /dll /out:$(OBJDIR)one.dll
	$(CC) /Fe$@ dltestwin.c  /nologo $(APP_CXXFLAGS) 

$(OBJDIR)unloadtest: unloadtest.c one.c two.c 
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)two.so -fpic -shared two.c -g
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)one.so -fpic -shared one.c -g
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)unloadtest $(APP_CXXFLAGS) unloadtest.c $(DL_LIB) -Wl,-rpath,`pwd`/$(OBJDIR) -g

$(OBJDIR)unloadtestwin.exe: unloadtestwin.c onewin.c twowin.c 
	$(CC) twowin.c  /nologo $(APP_CXXFLAGS) /link /dll /out:two.dll
	$(CC) onewin.c  /nologo $(APP_CXXFLAGS) /link /dll /out:one.dll
	$(CC) /Fe$@ unloadtestwin.c  /nologo $(APP_CXXFLAGS) 

$(OBJDIR)heavy_stack_win.exe: heavy_stack_win.c
	$(CC) /Fe$@ heavy_stack_win.c  /nologo $(APP_CXXFLAGS) 

$(OBJDIR)ebxhandle: ebxhandle.c ebx.s
	$(CC) $(APP_CXXFLAGS) -g -o $@ ebxhandle.c ebx.s

$(OBJDIR)simplefoo: simplefoo.c simplebar.c $(OBJDIR)simplesp.o
	$(CC) $(PIN_LPATHS) $(APP_CXXFLAGS) -I../Include -I. -static -o $@ $+ -g

$(OBJDIR)simplesp.o: simplesp.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)thd_malloc: thd_malloc.c
	$(CC) $(APP_CXXFLAGS) -o $@ thd_malloc.c $(APP_PTHREAD)

$(OBJDIR)mttraceapp: mttraceapp.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I../../include -I. -o $(OBJDIR)mttraceapp mttraceapp.c -lpinapp -lpthread -g

$(OBJDIR)replaceapp$(EXEEXT): replaceapp.c $(OBJDIR)do_nothing_$(TARGET_OS)_$(TARGET).$(OBJEXT) replaceapp.def
	$(CC) $(APP_CXXFLAGS) replaceapp.c $(OBJDIR)do_nothing_$(TARGET_OS)_$(TARGET).$(OBJEXT) $(OUTEXE)$@ /link /def:replaceapp.def

$(OBJDIR)do_nothing_$(TARGET_OS)_$(TARGET).$(OBJEXT): do_nothing_$(TARGET_OS)_$(TARGET).asm
	$(MASM) /nologo /c /Fo$@ $<

$(OBJDIR)create_process_app$(EXEEXT): create_process_app.cpp
	$(CC) $(APP_CXXFLAGS) $< $(OUTEXE)$@

$(OBJDIR)win_child_process.exe: win_child_process.cpp
	$(CC) $(APP_CXXFLAGS) $< $(OUTEXE)$@

$(OBJDIR)exception_in_probe_app.exe: exception_in_probe_app.c
	$(CC) $(APP_CXXFLAGS) $< ${OUTEXE}$@

$(OBJDIR)exception_in_probed_call_app.exe: exception_in_probed_call_app.c
	$(CC) $(NO_OPTIMIZE) $(APP_CXXFLAGS) $< ${OUTEXE}$@

$(OBJDIR)exception_in_probed_call_after_app.exe: exception_in_probed_call_after_app.c
	$(CC) $(NO_OPTIMIZE) $(APP_CXXFLAGS) $< ${OUTEXE}$@

$(OBJDIR)exception_in_probed_call_cpp_app.exe: exception_in_probed_call_cpp_app.cpp
	$(CC) $(NO_OPTIMIZE) $(APP_CXXFLAGS) $< ${OUTEXE}$@

$(OBJDIR)exception_in_probed_call_cpp_after_app.exe: exception_in_probed_call_cpp_after_app.cpp
	$(CC) $(NO_OPTIMIZE) $(APP_CXXFLAGS) $< ${OUTEXE}$@

$(OBJDIR)signals_app: signals_app.cpp
	$(CXX) $(APP_CXXFLAGS) $< ${OUTEXE}$@

$(OBJDIR)syscall_yield64.o: syscall_yield64.asm
	as -o $@ $<

$(OBJDIR)syscall_in_probe_app: syscall_in_probe_app.c $(OBJDIR)syscall_yield64.o
	$(CC) $(NO_OPTIMIZE) $(APP_CXXFLAGS) $< $(OBJDIR)syscall_yield64.o ${OUTEXE}$@


$(OBJDIR)spin_lock_app: spin_lock_app.c
	$(CC) $(APP_CXXFLAGS) -g spin_lock_app.c -o $@ -lpthread

$(OBJDIR)sempost_app: sempost_app.cpp
	$(CXX) $(APP_CXXFLAGS) -g $< -o $@ -lpthread

$(OBJDIR)svcraw_app: svcraw_app.cpp
	$(CXX) $(APP_CXXFLAGS) -g $< -o $@ 

$(OBJDIR)exit_app: exit_app.cpp
	$(CXX) $(APP_CXXFLAGS) -g $< -o $@ 

$(OBJDIR)relocate_app: relocate_app.cpp relocate_asm_$(TARGET).s
	$(CC) $(APP_CXXFLAGS) relocate_asm_$(TARGET).s -c -o $(OBJDIR)relocate_asm_$(TARGET).o
	$(CXX) $(APP_CXXFLAGS) -g $< $(OBJDIR)relocate_asm_$(TARGET).o -o $@ 

$(OBJDIR)error-app: error-main.c error-sub.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ error-main.c error-sub.c -g

$(OBJDIR)error-app.exe: error-main.c error-sub.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ error-main.c error-sub.c

$(OBJDIR)insert_call_probed_app: insert_call_probed_app.c
	$(CC) $(APP_CXXFLAGS) $< -o $@

##############################
#                            #
#     Tools                  #
#                            #
##############################

probe1.test : $(OBJDIR)probe1$(SUF) probe1.tested probe1.failed $(REPLACEAPP)
	$(PIN) -t $< -- ./$(REPLACEAPP) >  $<.out 2>&1
	grep "Nothing" probe1.outfile
	rm -f probe1.failed  $<.out

#probe2 is a negative test.
probe2.test : $(OBJDIR)probe2$(SUF) probe2.tested probe2.failed $(ERRREPLACE)
	-$(PIN) -t $< -- ./$(ERRREPLACE) >  $<.out 2>&1
	echo "probe2 should fail.  Ignore the error."
	rm -f probe2.failed  $<.out

probe3.test : $(OBJDIR)probe3$(SUF) probe3.tested probe3.failed $(REPLACECALL)
	$(PIN) -t $< -- ./$(REPLACECALL) >  $<.out 2>&1
	grep "Nothing" probe3.outfile
	rm -f probe3.failed  $<.out

#probe4 works correctly on ia32 with probe size = 5. negative test on intel64.
probe4.test : $(OBJDIR)probe4$(SUF) probe4.tested probe4.failed $(ERRCALL)
	-$(PIN) -t $<  -xyzzy -mesgon log_probe -logfile $<.log -- ./$(ERRCALL) >  $<.out 2>&1
	echo "probe4 should fail.  Ignore the error."
	grep call_function $<.log
	rm -f probe4.failed  $<.out $<.log

probe5.test : $(OBJDIR)probe5$(SUF) probe5.tested probe5.failed $(REPLACEFUN)
	$(PIN) -t $< -- ./$(REPLACEFUN) >  $<.out 2>&1
	grep "Nothing" probe5.outfile
	rm -f probe5.failed  $<.out

probe6.test : $(OBJDIR)probe6$(SUF) probe6.tested probe6.failed $(REPLACEDISP)
	$(PIN) -t $< -- ./$(REPLACEDISP) >  $<.out 2>&1
	grep "Doing" probe6.outfile
	rm -f probe6.failed  $<.out


probe7.test : $(OBJDIR)probe7$(SUF) probe7.tested probe7.failed $(REPLACEJMP)
	-$(PIN) -t $< -xyzzy -mesgon log_probe -- ./$(REPLACEJMP) >  $<.out 2>&1
	grep "Doing" probe7.outfile
	rm -f probe7.failed  $<.out

#probe8 is a negative test.
probe8.test : $(OBJDIR)probe8$(SUF) probe8.tested probe8.failed $(BADJUMP)
	-$(PIN) -t $<  -xyzzy -mesgon log_probe -logfile $<.log -- ./$(BADJUMP) >  $<.out 2>&1
	echo "probe8 should fail.  Ignore the error."
	grep unconditional $<.log
	rm -f probe8.failed  $<.out $<.outfile $<.log


probe9.test : $(OBJDIR)probe9$(SUF) probe9.tested probe9.failed $(REPLACESHORTY)
	-$(PIN) -t $< -xyzzy -mesgon log_probe -- ./$(REPLACESHORTY) >  $<.out 2>&1
	grep "Short" probe9.outfile
	rm -f probe9.failed  $<.out

probe10.test : $(OBJDIR)probe10$(SUF) probe10.tested probe10.failed $(GOODJUMP)
	-$(PIN)  -t $< -xyzzy -mesgon log_probe  -- ./$(GOODJUMP) >  $<.out 2>&1
	grep "replacement" $<.out
	rm -f probe10.failed $<.out

#probe_err1 is a negative test.
probe_err1.test : $(OBJDIR)probe_err1$(SUF) probe_err1.tested probe_err1.failed $(BADJUMP)
	-$(PIN) -error_file $(@:.test=.xml) -t $< -logfile $(@:.test=.log)  -- ./$(BADJUMP) >  $<.out 2>&1
	echo "probe_err1 should fail.  Ignore the error."
	grep -e "not suitable" $(@:.test=.xml)
	rm -f probe_err1.failed $<.out $(@:.test=.xml) $(@:.test=.log)

#probe_err2 is a negative test.
probe_err2.test : $(OBJDIR)probe_err2$(SUF) probe_err2.tested probe_err2.failed $(LITTLE_MALLOC)
	-$(PIN) -error_file $(@:.test=.xml) -t $< -logfile $(@:.test=.log) -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	echo "probe_err2 should fail.  Ignore the error."
	grep -e "not supported" $(@:.test=.xml)
	rm -f probe_err2.failed $<.out $(@:.test=.xml) $(@:.test=.log)


#probe_movip test check relocation of procedure with ip-relative mov instruction
probe_movip.test : %.test: $(OBJDIR)probe_movip$(SUF) %.tested %.failed $(MOVE_IP)
	-$(PIN)  -t $< -logfile $*.log  -- ./$(MOVE_IP) >  $*.out 2>&1
	rm -f $*.failed $*.out 

#probe_err4 is a negative test.
probe_err4.test : $(OBJDIR)probe_err4$(SUF) probe_err4.tested probe_err4.failed $(FALL_THRU)
	-$(PIN) -error_file $(@:.test=.xml) -t $< -logfile $(@:.test=.log)  -- ./$(FALL_THRU) >  $<.out 2>&1
	echo "probe_err4 should fail.  Ignore the error."
	grep -e "not suitable" $(@:.test=.xml)
	rm -f probe_err4.failed $<.out $(@:.test=.xml) $(@:.test=.log)

#probe_err5 is a negative test.
probe_err5.test : $(OBJDIR)probe_err5$(SUF) probe_err5.tested probe_err5.failed $(BAD_CALL)
	-$(PIN) -error_file $(@:.test=.xml) -t $< -logfile $(@:.test=.log)  -- ./$(BAD_CALL) >  $<.out 2>&1
	echo "probe_err5 should fail.  Ignore the error."
	grep -e "not suitable" $(@:.test=.xml)
	rm -f probe_err5.failed $<.out $(@:.test=.xml) $(@:.test=.log)


#probe_err6 is a negative test.
probe_err6.test : $(OBJDIR)probe_err6$(SUF) probe_err6.tested probe_err6.failed $(HIGH_TARGET)
	-$(PIN) -error_file $(@:.test=.xml) -t $< -logfile $(@:.test=.log)  -- ./$(HIGH_TARGET) >  $<.out 2>&1
	echo "probe_err6 should fail.  Ignore the error."
	grep -e "not suitable" $(@:.test=.xml)
	rm -f probe_err6.failed $<.out $(@:.test=.xml) $(@:.test=.log)


probe_on_probe1.test : %.test: $(OBJDIR)%$(SUF) %.tested %.failed $(REPLACEAPP)
	$(PIN) -t $< -o $*.out -- ./$(REPLACEAPP) >  $<.out 2>&1
	$(PIN_DIFF) $*.out $*.reference
	rm -f $*.failed  $<.out $*.out

probe_on_probe2.test : %.test: $(OBJDIR)%$(SUF) %.tested %.failed $(CREATE_PROCESS_APP) $(OBJDIR)win_child_process.exe
	$(PIN) -follow_execv -probe -t $< -o $*.out -- ./$(CREATE_PROCESS_APP) >  $<.out 2>&1
	grep -q correct $<.out
	grep -q "Pin Wrapper works" $*.out
	grep "CreateProcess" $*.out |grep -q before
	grep "CreateProcess" $*.out |grep -q after
	rm -f $*.failed  $<.out $*.out

probemalloctrace.test : $(OBJDIR)probemalloctrace$(SUF) probemalloctrace.tested probemalloctrace.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out 2>&1
	diff makefile $<.makefile.copy
	grep "Probe" probemalloctrace.outfile
	rm -f probemalloctrace.failed  $<.out $<.makefile.copy

probecdecl.test : $(OBJDIR)probecdecl$(SUF) probecdecl.tested probecdecl.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out 2>&1
	diff makefile $<.makefile.copy
	grep "Probe" probecdecl.outfile
	rm -f probecdecl.failed  $<.out $<.makefile.copy

probeheapalloc.test : $(OBJDIR)probeheapalloc$(SUF) probeheapalloc.tested probeheapalloc.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out 2>&1
	diff makefile $<.makefile.copy
	grep "Probe" probeheapalloc.outfile
	rm -f probeheapalloc.failed  $<.out $<.makefile.copy

probestdcall.test : $(OBJDIR)probestdcall$(SUF) probestdcall.tested probestdcall.failed $(OBJDIR)probestdcall_app
	$(PIN) -t $< -- $(OBJDIR)probestdcall_app >  $<.out 2>&1
	grep  "SUCCESS" $<.out
	rm -f $<.out
	# Enforce debug info garbage collection in symbol server
	$(PIN) -t $< -xyzzy -debug_info_max_size 0 -- $(OBJDIR)probestdcall_app >  $<.out 2>&1
	grep  "SUCCESS" $<.out
	rm -f probestdcall.failed  $<.out

probefastcall.test : $(OBJDIR)probefastcall$(SUF) probefastcall.tested probefastcall.failed $(OBJDIR)probefastcall_app
	$(PIN) -t $< -- $(OBJDIR)probefastcall_app >  $<.out 2>&1
	grep  "SUCCESS" $<.out
	rm -f $<.out
	# Enforce debug info garbage collection in symbol server
	$(PIN) -t $< -xyzzy -debug_info_max_size 0 -- $(OBJDIR)probefastcall_app >  $<.out 2>&1
	grep  "SUCCESS" $<.out
	rm -f probefastcall.failed  $<.out

code_range.test : $(OBJDIR)code_range$(SUF) code_range.tested code_range.failed $(APP_TRACE)
	$(PIN) -t $< -- ./$(APP_TRACE) >  $<.out 2>&1
	grep "Success" $<.out
	rm -f code_range.failed  $<.out

proto1.test : $(OBJDIR)proto1$(SUF) proto1.tested proto1.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out 2>&1
	diff makefile $<.makefile.copy
	rm -f proto1.failed  $<.out $<.makefile.copy

proto1win.test : $(OBJDIR)proto1win$(SUF) proto1win.tested proto1win.failed $(OBJDIR)dltestwin.exe
	$(PIN) -t $< -- ./$(OBJDIR)dltestwin >  $<.out 2>&1
	grep "Probe_Malloc" $<.out
	rm -f proto1win.failed  $<.out

#this is a negative test
proto1win-err.test : $(OBJDIR)proto1win-err$(SUF) proto1win-err.tested proto1win-err.failed
	echo "proto1win-err should fail.  Ignore the error."
	-$(PIN) -t $< -- ./$(OBJDIR)non-existent-test >  $<.out 2>&1
	grep -e "No such file or directory" $<.out
	rm -f proto1win-err.failed  $<.out

proto2.test : $(OBJDIR)proto2$(SUF) $(OBJDIR)protofoo proto2.tested proto2.failed
	$(PIN) -t $< -- ./$(OBJDIR)protofoo >  $<.out 2>&1
	grep -e "Original Arguments = ( 11, 22, 33, 44 )" $<.out
	grep -e "New Arguments = ( 100, 200, 300, 400, 500 )" $<.out
	rm -f proto2.failed  $<.out

replacesig_empty.test : $(OBJDIR)replacesig_empty$(SUF) $(OBJDIR)empty replacesig_empty.tested replacesig_empty.failed
	$(PIN) -t $< -- ./$(OBJDIR)empty >  $<.out 2>&1
	grep -e "33" $<.out
	rm -f replacesig_empty.failed  $<.out

replace_empty.test : $(OBJDIR)replace_empty$(SUF) $(OBJDIR)empty replace_empty.tested replace_empty.failed
	$(PIN) -t $< -- ./$(OBJDIR)empty >  $<.out 2>&1
	grep -e "33" $<.out
	rm -f replace_empty.failed  $<.out

replace_empty_twice.test : %.test : $(OBJDIR)%$(SUF) $(OBJDIR)empty %.tested %.failed
	$(PIN) -t $< -- ./$(OBJDIR)empty >  $<.out 2>&1
	grep -e "33" $<.out
	grep -q Boo2 $<.out
	grep -q Boo1 $<.out
	rm -f $*.failed  $<.out

proto3.test : $(OBJDIR)proto3$(SUF) proto3.tested proto3.failed
	$(PIN) -probe -t $< -- $(CP) makefile $<1.makefile.copy >  $<1.out 2>&1
	diff makefile $<1.makefile.copy
	grep -q "Probe_Malloc" $<1.out
	$(PIN) -t $< -- $(CP) makefile $<2.makefile.copy >  $<2.out 2>&1
	diff makefile $<2.makefile.copy
	grep -q "Jit_Malloc" $<2.out
	rm -f proto3.failed  $<1.out $<2.out $<1.makefile.copy $<2.makefile.copy

proto3win.test : $(OBJDIR)proto3win$(SUF) proto3win.tested proto3win.failed $(OBJDIR)dltestwin.exe
	$(PIN) -probe -t $< -- ./$(OBJDIR)dltestwin.exe >  $<1.out 2>&1
	grep -q "Probe_Malloc" $<1.out
	$(PIN) -t $< -- ./$(OBJDIR)dltestwin.exe >  $<2.out 2>&1
	grep -q "Jit_Malloc" $<2.out
	rm -f proto3win.failed  $<1.out $<2.out $<1.makefile.copy $<2.makefile.copy

proto4.test : $(OBJDIR)proto4$(SUF) proto4.tested proto4.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<1.out 2>&1
	grep "passed" $<1.out
	$(PIN) -t $< -- ./$(LITTLE_MALLOC) >  $<2.out 2>&1
	grep "passed" $<2.out
	rm -f proto4.failed  $<1.out $<2.out

insert1.test : $(OBJDIR)insert1$(SUF) insert1.tested insert1.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	rm -f insert1.failed  $<.out

insert1_no_xmm.test : %.test: $(OBJDIR)insert1$(SUF) %.tested %.failed $(LITTLE_MALLOC)
	$(PIN) -probe -xyzzy -mesgon log_final -logfile $*.log -bridge_save_fp 0 -t $< -- ./$(LITTLE_MALLOC) >   $*.out 2>&1
	grep "passed" $*.out
	test `grep xmm $*.log | wc -l` -eq "0"
	rm -f $*.failed $*.log $*.out

insert2.test : $(OBJDIR)insert2$(SUF) insert2.tested insert2.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	grep -q -e "Before 3" $<.out
	rm -f insert2.failed  $<.out

insert3.test : $(OBJDIR)insert3$(SUF) insert3.tested insert3.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<1.out 2>&1
	grep "passed" $<1.out
	grep -q -e "Before_MyMalloc" $<1.out
	grep -q -e "Before_MyFree" $<1.out
	rm -f insert3.failed  $<1.out

insert4.test : $(OBJDIR)insert4$(SUF) insert4.tested insert4.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	grep -q -e "Before_Malloc" $<.out
	grep -q -e "Before_Free" $<.out
	rm -f insert4.failed  $<.out

insert5.test : $(OBJDIR)insert5$(SUF) insert5.tested insert5.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	grep -q -e "Before_Malloc" $<.out
	grep -q -e "After_Malloc" $<.out
	rm -f insert5.failed  $<.out

insert6.test : $(OBJDIR)insert6$(SUF) insert6.tested insert6.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	grep -q -e "Before_Malloc" $<.out
	grep -q -e "After_Malloc" $<.out
	rm -f insert6.failed  $<.out

insert7.test : $(OBJDIR)insert7$(SUF) insert7.tested insert7.failed $(OBJDIR)protofoo
	$(PIN) -probe -t $< -- ./$(OBJDIR)protofoo >  $<.out 2>&1
	grep -q -e "345" $<.out
	grep -q -e "Before" $<.out
	grep -q -e "After" $<.out
	rm -f insert7.failed  $<.out

insert8.test : $(OBJDIR)insert8$(SUF) insert8.tested insert8.failed $(SIMPLEFOO10)
	$(PIN) -probe -t $< -- ./$(SIMPLEFOO10) >  $<.out 2>&1
	grep -q -e "one = 1" $<.out
	grep -q -e "Before: original arguments = ( 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 )" $<.out
	grep -q -e "After: return value = 45" $<.out
	grep -q -e "main: total = 45" $<.out
	rm -f insert8.failed  $<.out

insert9.test : $(OBJDIR)insert9$(SUF) insert9.tested insert9.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	grep -q -e "After_Malloc" $<.out
	rm -f insert9.failed  $<.out

insert10.test : $(OBJDIR)insert10$(SUF) insert10.tested insert10.failed $(SIMPLEFOO10)
	$(PIN) -probe -t $< -- ./$(SIMPLEFOO10) >  $<.out 2>&1
	grep -q -e "After: return value = 45" $<.out
	grep -q -e "main: total = 45" $<.out
	rm -f insert10.failed  $<.out

insert11.test : $(OBJDIR)insert11$(SUF) insert11.tested insert11.failed $(SIMPLEFOO9)
	$(PIN) -probe -t $< -- ./$(SIMPLEFOO9) >  $<.out 2>&1
	grep -q -e "one = 1" $<.out
	grep -q -e "Before: original arguments = ( 1, 2, 3, 4, 5, 6, 7, 8, 9 )" $<.out
	grep -q -e "After: return value = 45" $<.out
	grep -q -e "main: total = 45" $<.out
	rm -f insert11.failed  $<.out

insert12.test : $(OBJDIR)insert12$(SUF) insert12.tested insert12.failed $(SIMPLEFOO9)
	$(PIN) -probe -t $< -- ./$(SIMPLEFOO9) >  $<.out 2>&1
	rm -f insert12.failed  $<.out

insertfast.test : $(OBJDIR)insertfast$(SUF) insertfast.tested insertfast.failed $(OBJDIR)winfast.exe
	$(PIN) -probe -t $< -- ./$(OBJDIR)winfast.exe >  $<.out 2>&1
	grep -q -e "one = 1" $<.out
	grep -q -e "Before: arguments = ( 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 )" $<.out
	grep -q -e "After: return value = 45" $<.out
	grep -q -e "three = 3" $<.out
	rm -f insertfast.failed  $<.out

insertstd.test : $(OBJDIR)insertstd$(SUF) insertstd.tested insertstd.failed $(OBJDIR)winstd.exe
	$(PIN) -probe -t $< -- ./$(OBJDIR)winstd.exe >  $<.out 2>&1
	grep -q -e "one = 1" $<.out
	grep -q -e "Before: arguments = ( 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 )" $<.out
	grep -q -e "After: return value = 45" $<.out
	rm -f insertstd.failed  $<.out

insert1win.test : $(OBJDIR)insert1win$(SUF) insert1win.tested insert1win.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out
	diff makefile $<.makefile.copy
	grep "Before" $<.out
	rm -f insert1win.failed  $<.out $<.makefile.copy

insert2win.test : $(OBJDIR)insert2win$(SUF) insert2win.tested insert2win.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out
	diff makefile $<.makefile.copy
	grep -q -e "Before 3" $<.out
	rm -f insert2win.failed  $<.out $<.makefile.copy

insert3win.test : $(OBJDIR)insert3win$(SUF) insert3win.tested insert3win.failed
	$(PIN) -t $< -- $(CP) makefile $<.makefile.copy >  $<.out
	diff makefile $<.makefile.copy
	grep -q -e "Before_RtlAllocateHeap" $<.out
	rm -f insert3win.failed  $<.out $<.makefile.copy

debugtest.test : $(OBJDIR)debugtest$(SUF) debugtest.tested debugtest.failed $(LITTLE_MALLOC)
	$(PIN) -probe -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "passed" $<.out
	rm -f debugtest.failed  $<.out


iprtool.test : $(OBJDIR)iprtool$(SUF) iprtool.tested iprtool.failed $(IPR)
	$(PIN) -t $< -- ./$(IPR) >  $<.out 2>&1
	grep -e "a = 1" $<.out
	rm -f iprtool.failed  $<.out

probesleep.test : $(OBJDIR)probesleep$(SUF) probesleep.tested probesleep.failed $(SLEEP)
	$(PIN) -t $< -- ./$(SLEEP) >  $<.out 2>&1
	grep -e "SleepProbe" $<.out
	rm -f probesleep.failed  $<.out

atraceprobe.test : $(OBJDIR)atraceprobe$(SUF) atraceprobe.tested atraceprobe.failed $(OBJDIR)traceapp
	$(PIN) -t $< -- ./$(OBJDIR)traceapp
	rm -f  atraceprobe.failed

ssewin.test : %.test : $(OBJDIR)ssewin.dll %.tested %.failed $(OBJDIR)heavy_stack_win.exe
	$(PIN) -probe -t $< -- ./$(OBJDIR)heavy_stack_win.exe > $<1.out 2>&1
	rm -f $<1.out
	$(PIN) -t $< -- ./$(OBJDIR)heavy_stack_win.exe > $<2.out 2>&1
	rm -f ssewin.failed $<2.out

malloctrace2win.test : %.test : $(OBJDIR)malloctrace2win.dll $(MALLOCWRAPPERS) %.tested %.failed $(DLTEST)
	$(PIN) -probe -t $< -- ./$(DLTEST) > $<1.out 2>&1
	grep -q free $<1.out
	rm -f $<1.out
	$(PIN) -t $< -- ./$(DLTEST) > $<2.out 2>&1
	grep -q malloc $<2.out
	rm -f malloctrace2win.failed $<2.out

# segment registers are causing failures in the jit version, so disable it for now.
# it does pass on older systems that do not use segment registers.
malloctrace2.test : $(OBJDIR)malloctrace2$(SUF) malloctrace2.tested malloctrace2.failed $(DLTEST) $(MALLOCWRAPPERS)
	$(PIN) -t $< -- ./$(DLTEST) > $<1.out 2>&1
	grep Allocated $<1.out
	rm -f $<1.out
	rm -f malloctrace2.failed

symbolnames.test : $(OBJDIR)symbolnames$(SUF) symbolnames.tested symbolnames.failed $(DLTEST) $(DLTEST_TP)
	$(PIN) -t $< -- ./$(DLTEST) > $<1.out
	grep one $<1.out
	grep two $<1.out
	$(PIN) -t $< -- ./$(DLTEST_TP) > $<2.out
	grep one $<2.out
	grep two $<2.out
	rm -f symbolnames.failed $<1.out $<2.out

replaceebx.test : $(OBJDIR)replaceebx$(SUF) replaceebx.tested replaceebx.failed $(OBJDIR)ebxhandle
	$(PIN) -t $< -- ./$(OBJDIR)ebxhandle > $<.out 2>&1
	cmp $<.out $<.reference
	rm -f replaceebx.failed

context_probe.test : $(OBJDIR)context_probe$(SUF) $(OBJDIR)simplefoo context_probe.tested context_probe.failed
	$(PIN) -t $< -- ./$(OBJDIR)simplefoo >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	rm context_probe.failed $<.out

malloctrace_locktest.test: %.test: $(OBJDIR)malloctrace_locktest$(SUF) $(OBJDIR)thd_malloc %.tested %.failed
	$(PIN) -t $< -- ./$(OBJDIR)thd_malloc
	rm malloctrace_locktest.failed malloctrace_locktest.out

malloctrace_tpbug.test: %.test :$(OBJDIR)malloctrace_tpbug$(SUF) $(OBJDIR)thd_malloc %.tested %.failed
	$(PIN) -t $< -- ./$(OBJDIR)thd_malloc > $<.out 2>&1
	grep -q "test complete" $<.out
	rm malloctrace_tpbug.failed $<.out

ifeq ($(TARGET_OS),b)
# there is no _init for libpthread on FreeBSD
init.test: $(OBJDIR)init$(SUF) init.tested init.failed $(OBJDIR)thd_malloc
	$(PIN) -t $< -- ./$(OBJDIR)thd_malloc >  $<.outfile 2>&1
	grep "libc " $<.outfile
	rm -f init.failed  $<.outfile
else
init.test: $(OBJDIR)init$(SUF) init.tested init.failed $(OBJDIR)thd_malloc
	$(PIN) -t $< -- ./$(OBJDIR)thd_malloc >  $<.outfile 2>&1
	grep "libc initialized" $<.outfile
	grep "libpthread initialized" $<.outfile
	rm -f init.failed  $<.outfile
endif

replace_exit.test : $(OBJDIR)replace_exit$(SUF) replace_exit.tested replace_exit.failed $(LITTLE_MALLOC)
	$(PIN) -t $< -- ./$(LITTLE_MALLOC) >  $<.out 2>&1
	grep "my_exit" $<.out
	rm -f replace_exit.failed $<.out

shortBB.test : $(OBJDIR)shortBB$(SUF) shortBB.tested shortBB.failed $(BBTEST)
	$(PIN) -t $< -- $(BBTEST) >  $<.out 2>&1
	grep  "7" $<.out
	rm -f shortBB.failed  $<.out

shortBB64.test : $(OBJDIR)shortBB64$(SUF) shortBB64.tested shortBB64.failed $(BBTEST)
	$(PIN) -t $< -- $(BBTEST) >  $<.out 2>&1
	grep  "7" $<.out
	rm -f shortBB64.failed  $<.out

remove_probe.test : $(OBJDIR)remove_probe$(SUF) remove_probe.tested remove_probe.failed $(UNLOADTEST)
	$(PIN)  -xyzzy -mesgon log_probe -logfile $<.log -t $< -- ./$(UNLOADTEST) > $<.out 2>&1
	grep Removing ./$<.log
	rm -f $<.out remove_probe.failed $<.log

remove_probe2.test : $(OBJDIR)remove_probe2$(SUF) remove_probe2.tested remove_probe2.failed $(UNLOADTEST)
	$(PIN)  -xyzzy -mesgon log_probe -logfile $<.log -t $< -logfile $<_tool.log -- ./$(UNLOADTEST) > $<.out 2>&1
	grep Removing ./$<.log
	rm -f $<.out remove_probe2.failed $<.log $<_tool.log

tpss.test : $(OBJDIR)tpss$(SUF) tpss.tested tpss.failed $(OBJDIR)dltestwin.exe
	$(PIN) -t $< -- ./$(OBJDIR)dltestwin.exe >  $<.out 2>&1
	rm -f tpss.failed  $<.out

spinlock_replsig.test : $(OBJDIR)spinlock_replsig$(SUF) spinlock_replsig.tested spinlock_replsig.failed $(OBJDIR)spin_lock_app
	$(PIN) -probe -t $< -- ./$(OBJDIR)spin_lock_app >  $<.out 2>&1
	grep -q SpinLock $<.out
	grep -q released $<.out
	rm -f spinlock_replsig.failed  $<.out

spinlock_repl.test : $(OBJDIR)spinlock_repl$(SUF) spinlock_repl.tested spinlock_repl.failed $(OBJDIR)spin_lock_app
	$(PIN) -probe -t $< -- ./$(OBJDIR)spin_lock_app >  $<.out 2>&1
	grep -q SpinLock $<.out
	grep -q released $<.out
	rm -f spinlock_repl.failed  $<.out

spinlock_insert.test : $(OBJDIR)spinlock_insert$(SUF) spinlock_insert.tested spinlock_insert.failed $(OBJDIR)spin_lock_app
	$(PIN) -probe -t $< -- ./$(OBJDIR)spin_lock_app >  $<.out 2>&1
	grep -q After $<.out
	grep -q released $<.out
	rm -f spinlock_insert.failed  $<.out

sempost_repl.test : %.test : $(OBJDIR)sempost_repl$(SUF) %.tested %.failed $(OBJDIR)sempost_app
	$(PIN) -t $< -- ./$(OBJDIR)sempost_app 
	rm -f $*.failed  

svcraw_repl.test : %.test : $(OBJDIR)svcraw_repl$(SUF) %.tested %.failed $(OBJDIR)svcraw_app
	$(PIN) -t $< -- ./$(OBJDIR)svcraw_app 
	rm -f $*.failed  

exit_repl.test : %.test : $(OBJDIR)exit_repl$(SUF) %.tested %.failed $(OBJDIR)exit_app
	$(PIN) -t $< -- ./$(OBJDIR)exit_app 
	rm -f $*.failed 
    
relocate_rtn.test:  %.test : $(OBJDIR)relocate_rtn$(SUF) $(OBJDIR)relocate_app %.tested %.failed
	$(PIN) -t $< -- ./$(OBJDIR)relocate_app 
	rm -f $*.failed 

exception_in_probe.test : $(OBJDIR)exception_in_probe$(SUF) exception_in_probe.tested exception_in_probe.failed $(OBJDIR)exception_in_probe_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probe_app.exe >  exception_in_probe.out 2>&1
	grep -c "Exception C0000005" exception_in_probe.out | grep "2"
	grep -c "Stack-Overflow in RtlLeaveCriticalSection replacement routine" exception_in_probe.out | grep "2"
	rm exception_in_probe.failed exception_in_probe.out

exception_in_probe_on_probe.test : $(OBJDIR)exception_in_probe_on_probe$(SUF) exception_in_probe_on_probe.tested exception_in_probe_on_probe.failed $(OBJDIR)exception_in_probed_call_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_app.exe >  exception_in_probe_on_probe.out 2>&1
	grep -i "Exception C0000005" exception_in_probe_on_probe.out
	rm exception_in_probe_on_probe.failed exception_in_probe_on_probe.out

exception_in_probed_call.test : $(OBJDIR)exception_in_probed_call$(SUF) exception_in_probed_call.tested exception_in_probed_call.failed $(OBJDIR)exception_in_probed_call_app.exe $(OBJDIR)exception_in_probed_call_cpp_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_app.exe >  exception_in_probed_call.out 2>&1
	grep -i "Exception C0000005" exception_in_probed_call.out
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_cpp_app.exe >  exception_in_probed_call_cpp.out 2>&1
	grep -i "Exception" exception_in_probed_call_cpp.out
	rm exception_in_probed_call.failed exception_in_probed_call.out exception_in_probed_call_cpp.out

exception_in_probe_sig.test : $(OBJDIR)exception_in_probe_sig$(SUF) exception_in_probe_sig.tested exception_in_probe_sig.failed $(OBJDIR)exception_in_probe_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probe_app.exe >  exception_in_probe_sig.out 2>&1
	grep -c "Exception C0000005" exception_in_probe_sig.out | grep "2"
	rm exception_in_probe_sig.failed exception_in_probe_sig.out

exception_in_probe_on_probe_sig.test : $(OBJDIR)exception_in_probe_on_probe_sig$(SUF) exception_in_probe_on_probe_sig.tested exception_in_probe_on_probe_sig.failed $(OBJDIR)exception_in_probed_call_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_app.exe >  exception_in_probe_on_probe_sig.out 2>&1
	grep -i "Exception C0000005" exception_in_probe_on_probe_sig.out
	rm exception_in_probe_on_probe_sig.failed exception_in_probe_on_probe_sig.out

exception_in_probed_call_sig.test : $(OBJDIR)exception_in_probed_call_sig$(SUF) exception_in_probed_call_sig.tested exception_in_probed_call_sig.failed $(OBJDIR)exception_in_probed_call_app.exe $(OBJDIR)exception_in_probed_call_cpp_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_app.exe >  exception_in_probed_call_sig.out 2>&1
	grep -i "Exception C0000005" exception_in_probed_call_sig.out
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_cpp_app.exe >  exception_in_probed_call_cpp_sig.out 2>&1
	grep -i "Exception" exception_in_probed_call_cpp_sig.out
	rm exception_in_probed_call_sig.failed exception_in_probed_call_sig.out exception_in_probed_call_cpp_sig.out

exception_in_probed_call_after.test : $(OBJDIR)exception_in_probed_call_after$(SUF) exception_in_probed_call_after.tested exception_in_probed_call_after.failed $(OBJDIR)exception_in_probed_call_after_app.exe $(OBJDIR)exception_in_probed_call_cpp_after_app.exe
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_after_app.exe >  exception_in_probed_call_after.out 2>&1
	grep -i "Exception C0000005" exception_in_probed_call_after.out
	grep -c -i "result=28" exception_in_probed_call_after.out | grep "3"
	$(PIN) -t $< -- $(OBJDIR)exception_in_probed_call_cpp_after_app.exe >  exception_in_probed_call_cpp_after.out 2>&1
	grep -i "Exception" exception_in_probed_call_cpp_after.out
	grep -c -i "result=28" exception_in_probed_call_after.out | grep "3"
	rm exception_in_probed_call_after.failed exception_in_probed_call_after.out exception_in_probed_call_cpp_after.out

signals.test: %.test :$(OBJDIR)signals$(SUF) $(OBJDIR)signals_app %.tested %.failed
	$(PIN) -follow_execv -t $< -- $(OBJDIR)signals_app > $*.out 2>&1
	$(OBJDIR)signals_app > $*_expected.out 2>&1
	$(PIN_CMP) $*.out $*_expected.out
	rm $*.failed $*.out $*_expected.out

syscall_in_probe.test: %.test: $(OBJDIR)syscall_in_probe$(SUF) $(OBJDIR)syscall_in_probe_app %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)syscall_in_probe_app 
	rm $*.failed


load_map.test: %.test: $(OBJDIR)%$(SUF) $(OBJDIR)load_map_app %.tested %.failed
	export LD_LIBRARY_PATH=`pwd`\/$(OBJDIR); $(PIN) -t $< -- $(OBJDIR)load_map_app > $*.out 2>&1
	grep "2 routines were instrumented" $*.out
	rm $*.failed $*.out


error-lin-jit.test : $(OBJDIR)error-lin-jit$(PINTOOL_SUFFIX) $(ERROR_APP) error-lin-jit.tested error-lin-jit.failed
	$(PIN) -t $< -- ./$(ERROR_APP) >  $<.out 2>&1
	grep "Tool: errno=2" $<.out
	rm error-lin-jit.failed $<.out

error-lin-probe.test : $(OBJDIR)error-lin-probe$(PINTOOL_SUFFIX) $(ERROR_APP) error-lin-probe.tested error-lin-probe.failed
	$(PIN) -t $< -- ./$(ERROR_APP) >  $<.out 2>&1
	grep "Tool: errno=2" $<.out
	rm error-lin-probe.failed $<.out

error-win-jit.test : $(OBJDIR)error-win-jit$(PINTOOL_SUFFIX) $(ERROR_APP) error-win-jit.tested error-win-jit.failed
	$(PIN) -t $< -- ./$(ERROR_APP) >  $<.out 2>&1
	grep "Tool: error code=2" $<.out
	rm error-win-jit.failed $<.out

error-win-probe.test : $(OBJDIR)error-win-probe$(PINTOOL_SUFFIX) $(ERROR_APP) error-win-probe.tested error-win-probe.failed
	$(PIN) -t $< -- ./$(ERROR_APP) >  $<.out 2>&1
	grep "Tool: error code=2" $<.out
	rm error-win-probe.failed $<.out

insert_call_probed.test: %.test: $(OBJDIR)%$(SUF) $(OBJDIR)insert_call_probed_app %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)insert_call_probed_app > $*.out
	grep -i Notification $*.out > $*.res.out
	$(PIN_CMP) $*.res.out $*.reference
	rm $*.failed $*.out $*.res.out


## build rules

$(OBJDIR)%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS)  $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)

## cleaning
clean:
	-rm -rf $(OBJDIR) *.out *.tested *.failed pin.log pintool.log *.outfile *.xml

-include *.d

