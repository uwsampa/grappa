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
    STATIC=-static
endif

ifeq ($(TARGET_COMPILER),ms)
    include ../makefile.ms.config
    DBG?=
endif

ifneq ($(ENABLE_VS), 1)
    VS_FLAG = -xyzzy -virtual_segments 1
else
    VS_FLAG =
endif

SUF = $(PINTOOL_SUFFIX)

###============================================================
##
## Set up the test tools
##

TEST_TOOLS = 
## linux only
ifeq ($(TARGET_OS),l)
	TEST_TOOLS += fork_jit_tool fork_probed_tool follow_child unix_parent_tool  null_arg_check_tool bind_now_tool simple_tool
endif

# FreeBSD only
ifeq ($(TARGET_OS),b)
	TEST_TOOLS += fork_jit_tool fork_probed_tool follow_child unix_parent_tool  null_arg_check_tool
endif

# mac only
ifeq ($(TARGET_OS),m)
	TEST_TOOLS += fork_jit_tool 
endif

#windows only
ifeq ($(TARGET_OS),w)
	ifeq ($(TARGET_COMPILER),ms)
		TEST_TOOLS += follow_child_3gen_tool follow_child_3gen_tool_initsym grand_parent_tool parent_tool follow_child_tool
	endif
endif


###===========================================================

TOOLS = $(TEST_TOOLS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

## TESTS_TO_RUN includes all tools + some tests without tools

# linux only
ifeq ($(TARGET_OS),l)
	TESTS_TO_RUN += badexec.test fork_jit.test sigchld.test bind_now.test
	TESTS_TO_RUN += follow_execv_with_config1.test \
                        follow_execv_with_config2.test \
                        follow_execv_with_config3.test \
                        apparmor.test
    ifeq ($(PROBE),1)
        ifneq ($(TARGET),ipf)
            TESTS_TO_RUN += fork_probed.test vfork_probed.test fork_probed_static.test vfork_probed_static.test
        endif
        TESTS_TO_RUN += follow_execv_with_config1_probe.test \
                        follow_execv_with_config2_probe.test \
                        follow_execv_with_config3_probe.test 
    endif

    TESTS_TO_RUN += unique_logfile.test
    TESTS_TO_RUN += injectchild.test injectparent.test

    ifeq ($(TARGET),ia32) ## linux ia32

        ifneq ($(HOST_ARCH),ia32e) ## IA32 on IA32 (not IA32 on Intel64)
            x = `touch g1`
            #can't run script cross-platform
            TESTS_TO_RUN += unix_launcher.test null_argument.test
            ifeq ($(PROBE),1)
                TESTS_TO_RUN += null_argument_probed.test execve_errno.test sigchld2.test
            endif
        endif
    else
        TESTS_TO_RUN += unix_launcher.test null_argument.test
        ifeq ($(PROBE),1)
            TESTS_TO_RUN += null_argument_probed.test sigchld2.test
            ifneq ($(TARGET),ipf)
                TESTS_TO_RUN += execve_errno.test
            endif
         endif
    endif

    APPLICATIONS = fork_app vfork_app fork_app_static vfork_app_static parent_process child_process injection_test_app sigchld_app null_check_app1 null_check_app2 bind_now_app badexec
endif

# FreeBSD only
ifeq ($(TARGET_OS),b)
	TESTS_TO_RUN += fork_jit.test badexec.test injectparent.test unique_logfile.test
	TESTS_TO_RUN += follow_execv_with_config1.test follow_execv_with_config2.test follow_execv_with_config3.test
    TESTS_TO_RUN += null_argument.test
    ifeq ($(PROBE),1)
		TESTS_TO_RUN += fork_probed.test vfork_probed.test
        TESTS_TO_RUN += follow_execv_with_config1_probe.test follow_execv_with_config2_probe.test 
        TESTS_TO_RUN += follow_execv_with_config3_probe.test
        TESTS_TO_RUN += null_argument_probed.test execve_errno.test
    endif

    TESTS_TO_RUN += injectchild.test

    TESTS_TO_RUN += unix_launcher.test

    APPLICATIONS = fork_app vfork_app parent_process child_process injection_test_app null_check_app1 null_check_app2
    APPLICATIONS += badexec
endif

# mac
ifeq ($(TARGET_OS),m)
    TESTS_TO_RUN += fork_jit.test
    APPLICATIONS = fork_app
endif

# windows only
ifeq ($(TARGET_OS),w)
    TESTS_TO_RUN += win_parent_process1.test  win_parent_process1_probed.test
    TESTS_TO_RUN += win_parent_process1_unicode_probed.test
    TESTS_TO_RUN += win_parent_process2_jjj.test win_parent_process2_jjp.test
    TESTS_TO_RUN += win_parent_process2_jpj.test win_parent_process2_jpp.test
    TESTS_TO_RUN += win_parent_process2_pjj.test win_parent_process2_pjp.test
    TESTS_TO_RUN += win_parent_process2_ppj.test win_parent_process2_ppp.test
    TESTS_TO_RUN += win_parent_process2_unicode_ppp.test
    TESTS_TO_RUN += win_parent_process2_mt_ppp.test win_parent_process1_unicode_probed_withsym.test
    TESTS_TO_RUN += win_early_termination.test win_early_termination_debugged.test win_early_termination_probed.test win_early_termination_probed_debugged.test

    APPLICATIONS = win_parent_process win_child_process win_launcher_process win_launcher_debugged_process win_early_termination
endif

TEST_APPLICATIONS = $(APPLICATIONS:%=$(OBJDIR)%)


all: $(OBJDIR) $(TOOLS) $(TEST_APPLICATIONS)

## sanity

SANITY_TESTS = $(TESTS_TO_RUN)


tests-sanity: $(OBJDIR) $(SANITY_TESTS)


test: $(OBJDIR) $(TESTS_TO_RUN)

$(OBJDIR):
	mkdir -p $(OBJDIR)


injectchild.test : $(OBJDIR)injection_test_app injectchild.tested injectchild.failed
	$(PIN) -injection child -- $(OBJDIR)injection_test_app ch_file
	rm injectchild.failed

injectparent.test : %.test: $(OBJDIR)injection_test_app %.tested %.failed
	$(PIN) -injection parent -- $(OBJDIR)injection_test_app pa_file
	rm $*.failed

# This test is not run by default because it is expected to fail sometimes.  The "self"
# injection mode will fail if the kerel happens to load the Pin image in a range that
# conflicts with the application.  Since there is some randomness in the address ranges
# the kernel selects for Pin and it's shared libraries, this test will sometimes fail
# and sometimes succeed.  It appears to me (gmlueck) that it fails about 1 in 10 tries.
#
injectself.test : injectself.tested injectself.failed
	touch injectself.makefile.copy; rm injectself.makefile.copy
	$(PIN) -injection self -- $(TESTAPP) makefile injectself.makefile.copy
	$(PIN_DIFF) makefile injectself.makefile.copy
	rm injectself.failed injectself.makefile.copy

$(OBJDIR)injection_test_app: injection_test_app.c
	$(CC) $(APP_CXXFLAGS) $< -o $@

##======================================================
## "fork()" callbacks in JIT mode (Linux and Mac)
##======================================================
fork_jit.test : %.test: $(OBJDIR)fork_jit_tool$(SUF) $(OBJDIR)fork_app %.tested %.failed
	$(PIN) -t $< -o $(OBJDIR)$*.out -- ./$(OBJDIR)fork_app
	grep -q "correct in child process" $(OBJDIR)$*.out*
	grep -q "correct in parent process" $(OBJDIR)$*.out*
	rm $*.failed $(OBJDIR)$*.out*

fork_probed.test : %.test: $(OBJDIR)fork_probed_tool$(SUF) $(OBJDIR)fork_app %.tested %.failed
	$(PIN) -t $< -o $(OBJDIR)$*.out -- ./$(OBJDIR)fork_app
	grep -q "After fork in parent" $(OBJDIR)$*.out*
	grep -q "After fork in child" $(OBJDIR)$*.out*
	rm $*.failed $(OBJDIR)$*.out*

vfork_probed.test: %.test: $(OBJDIR)fork_probed_tool$(SUF) $(OBJDIR)vfork_app $(OBJDIR)null_check_app2 %.tested %.failed
	$(PIN) -follow_execv -t $< -o $(OBJDIR)$*.out --  ./$(OBJDIR)vfork_app $(OBJDIR)null_check_app2
	grep -q Before $(OBJDIR)$*.out*
	grep -q "After fork in parent" $(OBJDIR)$*.out*
	grep -q "After fork in child" $(OBJDIR)$*.out*
	grep -q "At follow child callback in child process"  $(OBJDIR)$*.out*
	grep -q "At follow child callback in parent process"  $(OBJDIR)$*.out*
	rm -f $*.failed $(OBJDIR)$*.out*
			
fork_probed_static.test : %.test: $(OBJDIR)fork_probed_tool$(SUF) $(OBJDIR)fork_app_static %.tested %.failed
	rm -f $(OBJDIR)$*.out*
	$(PIN) -t $< -o $(OBJDIR)$*.out -- ./$(OBJDIR)fork_app_static
	grep -q "After fork in parent" $(OBJDIR)$*.out*
	grep -q "After fork in child" $(OBJDIR)$*.out*
	rm $*.failed $(OBJDIR)$*.out*

vfork_probed_static.test: %.test: $(OBJDIR)fork_probed_tool$(SUF) $(OBJDIR)vfork_app_static $(OBJDIR)null_check_app2 %.tested %.failed
	rm -f $(OBJDIR)$*.out*
	$(PIN) -follow_execv -t $< -o $(OBJDIR)$*.out --  ./$(OBJDIR)vfork_app_static $(OBJDIR)null_check_app2
	grep -q Before $(OBJDIR)$*.out*
	grep -q "After fork in parent" $(OBJDIR)$*.out*
	grep -q "After fork in child" $(OBJDIR)$*.out*
	rm -f $*.failed $(OBJDIR)$*.out*
			
##======================================================
## Tests a non default behaviour upon signal SIGCHLD (Linux)
##======================================================
sigchld.test: %.test: $(OBJDIR)sigchld_app %.tested %.failed
	$(PIN) -follow_execv -injection parent -- $<
	$(PIN_DIFF) sigchld_app.out sigchld_app.reference
	$(PIN) -follow_execv -injection child -- $<
	rm $*.failed sigchld_app.out

sigchld2.test: %.test: $(OBJDIR)simple_tool.so uname_script.sh %.tested %.failed
	$(PIN) -follow_execv -injection parent -t $< -probe  -- ./uname_script.sh  $(OBJDIR)$*.out 
	uname -s > $(OBJDIR)uname_expected.out
	$(PIN_CMP) $(OBJDIR)uname_expected.out $(OBJDIR)$*.out
	$(PIN) -follow_execv -injection child -t $< -probe  -- ./uname_script.sh $(OBJDIR)$*_inj_child.out
	rm -f $(OBJDIR)uname_expected.out $(OBJDIR)$*.out $(OBJDIR)$*_inj_child.out $*.failed

##======================================================
## -follow_execv one level tests: (Linux and mac)
##======================================================
##
## (1) do not run child under the Pin
##
follow_execv_with_config1.test: %.test : $(OBJDIR)unix_parent_tool$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process %.failed %.tested
	$(PIN) -follow_execv 1 -t $< -xyzzy -app ./child_process_xxx -- ./$(OBJDIR)parent_process ./$(OBJDIR)child_process "param1 param2" param3 >$*.out
	## child_process_xxx is not equal to child_process, do not run child under Pin at all
	grep "Do not run Pin under the child process" $*.out
	## unix_parent_tool should appear 1 time in the output since it was used in parent only
	test `grep "In unix_parent_tool PinTool" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out

##
## (1-probe) do not run child under the Pin
##
follow_execv_with_config1_probe.test:  %.test :$(OBJDIR)unix_parent_tool$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process %.failed %.tested
	$(PIN) -follow_execv 1 -t $< -xyzzy -probe -app ./child_process_xxx -- ./$(OBJDIR)parent_process $(OBJDIR)child_process "param1 param2" param3 >$*.out
	## child_process_xxx is not equal to child_process, do not run child under Pin at all
	grep "Do not run Pin under the child process" $*.out
	## unix_parent_tool should appear 1 time in the output since it was used in parent only
	test `grep "In unix_parent_tool PinTool" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out

##
## (2) run child under the Pin, but don't change Pin command line
##
follow_execv_with_config2.test:  %.test : $(OBJDIR)unix_parent_tool$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process %.failed %.tested
	$(PIN) -follow_execv 1 -t $< -xyzzy -app ./$(OBJDIR)child_process -- ./$(OBJDIR)parent_process ./$(OBJDIR)child_process "param1 param2" param3 >$*.out
	## -app argument is equal to application name, run the child process under the Pin
	grep "Pin command line remains unchanged" $*.out
	## unix_parent_tool should appear 2 times in the output since it was used in the parent and in the child
	test `grep "In unix_parent_tool PinTool" $*.out | wc -l` -eq "2"
	rm $*.failed $*.out


##
## (2-probe) run child under the Pin, but don't change Pin command line
##
follow_execv_with_config2_probe.test: %.test :$(OBJDIR)unix_parent_tool$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process follow_execv_with_config2_probe.failed follow_execv_with_config2_probe.tested
	$(PIN) -follow_execv 1 -t $< -xyzzy -probe -app ./$(OBJDIR)child_process -- ./$(OBJDIR)parent_process ./$(OBJDIR)child_process "param1 param2" param3 >$*.out
	## -app argument is equal to application name, run the child process under the Pin
	grep "Pin command line remains unchanged" $*.out
	## unix_parent_tool should appear 2 times in the output since it was used in the parent and in the child
	test `grep "In unix_parent_tool PinTool" $*.out | wc -l` -eq "2"
	rm $*.failed $*.out


##
## (3) run child under the Pin, and change Pin command line
##
follow_execv_with_config3.test: %.test : $(OBJDIR)unix_parent_tool$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process follow_execv_with_config3.failed follow_execv_with_config3.tested $(OBJDIR)follow_child$(SUF)
	$(PIN) -follow_execv 1 -t $< -xyzzy -app ./$(OBJDIR)child_process -pin "$(PIN) -t ./$(OBJDIR)follow_child$(SUF)" -- ./$(OBJDIR)parent_process ./$(OBJDIR)child_process "param1 param2" param3 >$*.out
	## change Pin command line
	grep "Process to execute" $*.out
	## unix_parent_tool should appear 1 time in the output since it was used in the parent only
	test `grep "In unix_parent_tool PinTool" $*.out | wc -l` -eq "1"
	## follow_child tool should appear 1 time in the output since it was used in the child only
	test `grep "In follow_child PinTool" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out

##
## (3-probe) run child under the Pin, and change Pin command line
##
follow_execv_with_config3_probe.test: %.test : $(OBJDIR)unix_parent_tool$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process %.failed %.tested $(OBJDIR)follow_child$(SUF)
	$(PIN) -follow_execv 1 -t $< -xyzzy -probe -app ./$(OBJDIR)child_process -pin "$(PIN) -t ./$(OBJDIR)follow_child$(SUF)" -- ./$(OBJDIR)parent_process ./$(OBJDIR)child_process "param1 param2" param3 >$*.out
	## change Pin command line
	grep "Process to execute" $*.out
	## unix_parent_tool should appear 1 time in the output since it was used in the parent only
	test `grep "In unix_parent_tool PinTool" $*.out | wc -l` -eq "1"
	## follow_child tool should appear 1 time in the output since it was used in the child only
	test `grep "In follow_child PinTool" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out

##======================================================
## -follow_execv two level tests: (Linux and mac)
##======================================================

##
## launcher runs parent_process twice; parent_process runs child_process
##
unix_launcher.test: %.test : $(OBJDIR)follow_child$(SUF) $(OBJDIR)parent_process $(OBJDIR)child_process parent_process_launcher.sh %.failed %.tested
	./parent_process_launcher.sh $(OBJDIR)parent_process $(OBJDIR)child_process> unix_launcher.reference
	$(PIN) -follow_execv 1 -t $< -- /bin/sh ./parent_process_launcher.sh $(OBJDIR)parent_process $(OBJDIR)child_process > $*.out 
	grep -v PinTool $*.out > $*.out1
	grep PinTool $*.out > $*.out2
	$(PIN_DIFF) $*.out1 unix_launcher.reference
	## PinTool should appear 5 times: 
	## parent_process_launcher.sh - 1, parent_process - 2 child_process - 2
	test `grep PinTool $*.out|wc -l` -eq "5"
	rm unix_launcher.out* unix_launcher.reference $*.failed

##======================================================
## Execve with 0 in second argument execv(app, NULL)
##======================================================

null_argument.test: %.test : $(OBJDIR)null_arg_check_tool$(SUF) $(OBJDIR)null_check_app1 $(OBJDIR)null_check_app2 %.failed %.tested
	$(PIN) -follow_execv 1 -t $< -- $(OBJDIR)null_check_app1 $(OBJDIR)null_check_app2
	rm $*.failed

null_argument_probed.test: %.test : $(OBJDIR)null_arg_check_tool$(SUF) $(OBJDIR)null_check_app1 $(OBJDIR)null_check_app2 %.failed %.tested
	$(PIN) -follow_execv 1 -probe -t $< -- $(OBJDIR)null_check_app1 $(OBJDIR)null_check_app2
	rm $*.failed


##======================================================
## Execve fails and returns with error code
##======================================================

badexec.test : badexec.tested badexec.failed $(OBJDIR)badexec
	./$(OBJDIR)badexec > badexec.reference
	$(PIN) -xyzzy -follow_execv 1 -- ./$(OBJDIR)badexec > badexec.out 2>&1
	$(PIN_DIFF) badexec.out badexec.reference
	rm badexec.failed badexec.out badexec.reference 

##======================================================
## Execve fails and and sets errno
## Bash checks errno value after execve return
##======================================================
execve_errno.test : %.test: $(OBJDIR)unix_parent_tool.so 1.sh 2.sh %.tested %.failed
	$(PIN) -follow_execv -t $< -probe --app ./2.sh -- ./1.sh > $*.out 2>&1
	grep SUCCESS $*.out
	rm $*.out $*.failed

##======================================================
## Test for getpid() inside Pin; -unique_logfile calls to getpid()
##======================================================

unique_logfile.test: %.test : $(OBJDIR)parent_process $(OBJDIR)child_process %.failed %.tested
	rm -f pin.log.*
	$(PIN) -follow_execv 1 -xyzzy -mesgon log_syscall -unique_logfile -injection child -- $(OBJDIR)parent_process $(OBJDIR)child_process "param1 param2" param3 >$*.out
	test `ls pin.log.*|wc -l` -eq 2
	rm -f pin.log.*
	$(PIN) -follow_execv 1 -xyzzy -mesgon log_syscall -unique_logfile -injection parent -- $(OBJDIR)parent_process $(OBJDIR)child_process "param1 param2" param3 >$*.out
	test `ls pin.log.*|wc -l` -eq 2
	rm pin.log.* $*.out $*.failed
	


##======================================================
## Test for LD_BIND_NOW
##======================================================

bind_now.test: %.test : $(OBJDIR)bind_now_tool$(SUF) $(OBJDIR)bind_now_app %.failed %.tested
	rm -f bind_now.out
	$(PIN) -follow_execv -t $< -- $(OBJDIR)bind_now_app -child
	test `grep -c "LD_BIND_NOW was found" bind_now.out` -eq 5
	rm -f bind_now.out $*.failed

##===========================================================
## Test for Pin injection in application guarded by AppArmor
##===========================================================

apparmor.test: %.test : %.failed %.tested
	if test -f /etc/apparmor.d/bin.netstat && test -f /bin/netstat ; then \
	$(PIN) -- netstat -g > $*.out 2>&1 || grep AppArmor $*.out; fi
	if test -f /etc/apparmor.d/usr.sbin.nscd && test -f /usr/sbin/nscd ; then \
	$(PIN) -- /usr/sbin/nscd -V > $*.out 2>&1 || grep AppArmor $*.out; fi
	rm -f $*.out $*.failed
##======================================================
## -follow_execv Windows tests
##======================================================
##
## (1) run child under the Pin, but don't change Pin command line
##
win_parent_process1.test : $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process1.tested win_parent_process1.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process1.out; rm win_parent_process1.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t $< -- cmd /c win_parent_process_launcher.bat $(OBJDIR)>  win_parent_process1.out 2>&1
	$(PIN_DIFF) win_parent_process1.out win_parent_process1.reference
	rm win_parent_process1.failed win_parent_process1.out

win_parent_process1_probed.test : $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process1_probed.tested win_parent_process1_probed.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process1_probed.out; rm win_parent_process1_probed.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t $< -- cmd /c win_parent_process_launcher.bat $(OBJDIR)>  win_parent_process1_probed.out 2>&1
	$(PIN_DIFF) win_parent_process1_probed.out win_parent_process1_probed.reference
	rm win_parent_process1_probed.failed win_parent_process1_probed.out	

win_parent_process1_unicode_probed.test : $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process1_unicode_probed.tested win_parent_process1_unicode_probed.failed $(OBJDIR)win_parent_process_unicode $(OBJDIR)win_child_process
	touch win_parent_process1_unicode_probed.out; rm win_parent_process1_unicode_probed.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t $< -- cmd /c win_parent_process_unicode_launcher.bat $(OBJDIR)>  win_parent_process1_unicode_probed.out 2>&1
	$(PIN_DIFF) win_parent_process1_unicode_probed.out win_parent_process1_unicode_probed.reference
	rm win_parent_process1_unicode_probed.failed win_parent_process1_unicode_probed.out	

win_parent_process1_unicode_probed_withsym.test : $(OBJDIR)follow_child_3gen_tool_initsym$(SUF) win_parent_process1_unicode_probed_withsym.tested win_parent_process1_unicode_probed_withsym.failed $(OBJDIR)win_parent_process_unicode $(OBJDIR)win_child_process
	touch win_parent_process1_unicode_probed_withsym.out; rm win_parent_process1_unicode_probed_withsym.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t $< -- cmd /c win_parent_process_unicode_launcher.bat $(OBJDIR)>  win_parent_process1_unicode_probed_withsym.out 2>&1
	$(PIN_DIFF) win_parent_process1_unicode_probed_withsym.out win_parent_process1_unicode_probed_withsym.reference
	rm win_parent_process1_unicode_probed_withsym.failed win_parent_process1_unicode_probed_withsym.out
	
win_early_termination.test : %.test: $(OBJDIR)follow_child_tool$(SUF) $(OBJDIR)win_launcher_process $(OBJDIR)win_early_termination %.tested %.failed
	touch $*.out; rm $*.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t $< -- $(OBJDIR)win_launcher_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	touch $*.out; rm $*.out
	
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t $< -load_system_dlls 1 -- $(OBJDIR)win_launcher_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	
	
win_early_termination_debugged.test : %.test: $(OBJDIR)follow_child_tool$(SUF) $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination %.tested %.failed	
	touch $*.out; rm $*.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t $< -- $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "1"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	touch $*.out; rm $*.out
	
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t $< -load_system_dlls 1 -- $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "1"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out		
	
win_early_termination_probed.test : %.test: $(OBJDIR)follow_child_tool$(SUF) $(OBJDIR)win_launcher_process $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination %.tested %.failed
	touch $*.out; rm $*.out
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -probe -follow_execv -t $< -- $(OBJDIR)win_launcher_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	touch $*.out; rm $*.out
	
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -probe -follow_execv -t $< -load_system_dlls 1 -- $(OBJDIR)win_launcher_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out	
	rm $*.failed $*.out	
	
win_early_termination_probed_debugged.test : %.test: $(OBJDIR)follow_child_tool$(SUF) $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination %.tested %.failed		
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -probe -follow_execv -t $< -- $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "1"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	touch $*.out; rm $*.out
	
	$(PIN_EXE) $(PIN_TEST_FLAGS) -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -probe -follow_execv -t $< -load_system_dlls 1 -- $(OBJDIR)win_launcher_debugged_process $(OBJDIR)win_early_termination >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "1"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out		

##
## (2) run child processes under Pin for some processes (filter by name), and change Pin command line for them.
##
# j means jitted, p means probed - so win_parent_process2_jjp.test means:
# grandparent jitted, parent jitted, child probed
#jjj
win_parent_process2_jjj.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_jjj.tested win_parent_process2_jjj.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_jjj.out; rm win_parent_process2_jjj.out
	cmd /c win_parent_process_test_launcher.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t " "$< " " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_jjj.out 2>&1
	$(PIN_DIFF) win_parent_process2_jjj.out win_parent_process2_jjj.reference
	rm win_parent_process2_jjj.failed win_parent_process2_jjj.out

#jjp	
win_parent_process2_jjp.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_jjp.tested win_parent_process2_jjp.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_jjp.out; rm win_parent_process2_jjp.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t " "$<" "-probe_grand_child 1" " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_jjp.out 2>&1
	$(PIN_DIFF) win_parent_process2_jjp.out win_parent_process2_jjp.reference
	rm win_parent_process2_jjp.failed win_parent_process2_jjp.out	

#jpj
win_parent_process2_jpj.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_jpj.tested win_parent_process2_jpj.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_jpj.out; rm win_parent_process2_jpj.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t " "$<" "-probe_child 1" " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_jpj.out 2>&1
	$(PIN_DIFF) win_parent_process2_jpj.out win_parent_process2_jpj.reference
	rm win_parent_process2_jpj.failed win_parent_process2_jpj.out

#jpp
win_parent_process2_jpp.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_jpp.tested win_parent_process2_jpp.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_jpp.out; rm win_parent_process2_jpp.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -t " "$<"  "-probe_child 1 -probe_grand_child 1" " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_jpp.out 2>&1
	$(PIN_DIFF) win_parent_process2_jpp.out win_parent_process2_jpp.reference
	rm win_parent_process2_jpp.failed win_parent_process2_jpp.out	

#pjj
win_parent_process2_pjj.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_pjj.tested win_parent_process2_pjj.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_pjj.out; rm win_parent_process2_pjj.out
	cmd /c win_parent_process_test_launcher.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t " "$< " " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_pjj.out 2>&1
	$(PIN_DIFF) win_parent_process2_pjj.out win_parent_process2_pjj.reference
	rm win_parent_process2_pjj.failed win_parent_process2_pjj.out	

#pjp
win_parent_process2_pjp.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_pjp.tested win_parent_process2_pjp.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_pjp.out; rm win_parent_process2_pjp.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t " "$<" "-probe_grand_child 1" " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_pjp.out 2>&1
	$(PIN_DIFF) win_parent_process2_pjp.out win_parent_process2_pjp.reference
	rm win_parent_process2_pjp.failed win_parent_process2_pjp.out	

#ppj
win_parent_process2_ppj.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_ppj.tested win_parent_process2_ppj.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_ppj.out; rm win_parent_process2_ppj.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t " "$<" "-probe_child 1" " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_ppj.out 2>&1
	$(PIN_DIFF) win_parent_process2_ppj.out win_parent_process2_ppj.reference
	rm win_parent_process2_ppj.failed win_parent_process2_ppj.out	

#ppp
win_parent_process2_ppp.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_ppp.tested win_parent_process2_ppp.failed $(OBJDIR)win_parent_process $(OBJDIR)win_child_process
	touch win_parent_process2_ppp.out; rm win_parent_process2_ppp.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t " "$<" "-probe_child 1 -probe_grand_child 1" " -- cmd /c win_parent_process_launcher.bat $(OBJDIR)" >  win_parent_process2_ppp.out 2>&1
	$(PIN_DIFF) win_parent_process2_ppp.out win_parent_process2_ppp.reference
	rm win_parent_process2_ppp.failed win_parent_process2_ppp.out

win_parent_process2_unicode_ppp.test : $(OBJDIR)grand_parent_tool$(SUF) $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_unicode_ppp.tested win_parent_process2_unicode_ppp.failed $(OBJDIR)win_parent_process_unicode $(OBJDIR)win_child_process
	touch win_parent_process2_unicode_ppp.out; rm win_parent_process2_unicode_ppp.out
	cmd /c win_parent_process_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t " "$<" "-probe_child 1 -probe_grand_child 1" " -- cmd /c win_parent_process_unicode_launcher.bat $(OBJDIR)" >  win_parent_process2_unicode_ppp.out 2>&1
	$(PIN_DIFF) win_parent_process2_unicode_ppp.out win_parent_process2_unicode_ppp.reference
	rm win_parent_process2_unicode_ppp.failed win_parent_process2_unicode_ppp.out

#multi-threaded processes	

#ppp
win_parent_process2_mt_ppp.test :  $(OBJDIR)grand_parent_tool$(SUF)  $(OBJDIR)parent_tool$(SUF) $(OBJDIR)follow_child_3gen_tool$(SUF) win_parent_process2_mt_ppp.tested win_parent_process2_mt_ppp.failed $(OBJDIR)win_parent_process_mt $(OBJDIR)win_child_process
	touch win_parent_process2_mt_ppp.out; rm win_parent_process2_mt_ppp.out
	cmd /c win_parent_process_mt_test_launcher_w_pintool_args.bat $(PIN_EXE) "$(PIN_TEST_FLAGS)" "-p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64) -follow_execv -probe -t " "$<"  "-probe_child 1 -probe_grand_child 1" " -- cmd /c win_parent_process_mt_launcher.bat $(OBJDIR)" >  win_parent_process2_mt_ppp.out 2>&1
	$(PIN_DIFF) win_parent_process2_mt_ppp.out win_parent_process2_mt_ppp.reference
	rm win_parent_process2_mt_ppp.failed win_parent_process2_mt_ppp.out


##===================================================================================
## common and private build rules for tools and applications
##==================================================================================
$(OBJDIR)badexec: badexec.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)fork_app: fork_app.c
	$(CC) $(APP_CXXFLAGS) $(DBG) $< -o $@

$(OBJDIR)fork_app_static: fork_app.c
	$(CC) $(APP_CXXFLAGS) -g $< -o $@ $(STATIC) 

$(OBJDIR)vfork_app: vfork_app.c
	$(CC) $(APP_CXXFLAGS) $(DBG) $< -o $@
				
$(OBJDIR)vfork_app_static: vfork_app.c
	$(CC) $(APP_CXXFLAGS) -g $< -o $@ $(STATIC)
				
$(OBJDIR)sigchld_app: sigchld_app.cpp
	$(CXX) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)bind_now_app: bind_now_app.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

$(OBJDIR)libarglist.a: arglist.cpp arglist.h
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT} $(OBJDIR)arglist.o $<
	$(AR) cr $@ $(OBJDIR)arglist.o

$(OBJDIR)parent_process: parent_process.cpp
	$(CXX) $(APP_CXXFLAGS) $(DBG) ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) 

$(OBJDIR)child_process: child_process.cpp
	$(CXX)  ${APP_CXXFLAGS} $(DBG) ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)null_check_app1: null_check_app1.cpp
	$(CXX)  ${APP_CXXFLAGS} $(DBG) ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)null_check_app2: null_check_app2.cpp
	$(CXX)  ${APP_CXXFLAGS} $(DBG) ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_parent_process: win_parent_process.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) Advapi32.lib

$(OBJDIR)win_parent_process_mt: win_parent_process_mt.cpp 
	$(CXX)  ${APP_CXXFLAGS2} ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)  Advapi32.lib

$(OBJDIR)win_parent_process_unicode: win_parent_process_unicode.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) Advapi32.lib

$(OBJDIR)win_child_process: win_child_process.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)
	
$(OBJDIR)win_early_termination: win_early_termination.cpp $(OBJDIR)win_terminate_process_dll.dll
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) Advapi32.lib Dbghelp.lib $(OBJDIR)win_terminate_process_dll.lib
	
$(OBJDIR)win_terminate_process_dll.dll: win_terminate_process_dll.cpp
	$(CXX) $(APP_CXXFLAGS2)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS) /dll		



$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
	
$(OBJDIR)%.o : %.cc
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
	
$(OBJDIR)%.o : %.cc
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(STATIC_TOOLS): $(PIN_LIBNAMES)

$(OBJDIR)%.o : %.S
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

## overriding some build rules
$(OBJDIR)unix_parent_tool$(PINTOOL_SUFFIX): $(OBJDIR)unix_parent_tool.o $(OBJDIR)libarglist.a
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) $(OBJDIR)libarglist.a

$(OBJDIR)unix_parent_tool.o : unix_parent_tool.cpp arglist.h
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<


## cleaning
clean:
	-rm -rf $(OBJDIR) pin.log* *.tested *.failed *.out badexec.reference


