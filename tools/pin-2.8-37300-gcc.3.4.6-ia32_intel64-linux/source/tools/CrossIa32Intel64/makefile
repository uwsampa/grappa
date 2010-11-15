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
    ifneq ($(KIT),1)
        PIN+= -p32 $(PIN_EXE_32) -p64 $(PIN_EXE_64)
    endif
endif

#####################
#common configuration

OBJDIR_intel64 = obj-intel64
OBJDIR_ia32    = obj-ia32

PARENT_PROCESS = $(TARGET_OS)_parent_process
CHILD_PROCESS = $(TARGET_OS)_child_process
GRAND_CHILD_PROCESS = $(TARGET_OS)_grand_child_process
TOOL_ROOTS = follow_child_tool follow_child_tool_sym

APP_ROOTS = $(PARENT_PROCESS) $(CHILD_PROCESS) $(GRAND_CHILD_PROCESS)
TEST_ROOTS = cross_32_64_32 cross_64_32_64 

ifeq ($(TARGET_OS),l)
    TEST_ROOTS += environment space sigchld_cross
	APP_ROOTS += sigchld_app
    ifeq ($(PROBE),1)
        TEST_ROOTS += cross_32_64_32_probe space_probe
    endif
else
    TEST_ROOTS += cross_32_64_32_probe cross_64_64_32_log cross_32_32_64_probe_log 
    TEST_ROOTS += cross_32_64_32_sym cross_64_32_64_sym cross_32_64_32_probe_sym cross_64_32_64_probe_sym
    TEST_ROOTS += w_space w_cross_64_32_early_termination w_cross_64_32_early_termination_probed 
    TEST_ROOTS += w_cross_32_64_early_termination w_cross_32_64_early_termination_probed
    
    TOOL_ROOTS += w_follow_child_tool
endif


TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
APPS  = $(APP_ROOTS:%=$(OBJDIR)%)

#######################
#specific configuration

CROSS_COMPILE =
MAKE_PREFIX_X86 =
MAKE_PREFIX_AMD64 =
TESTS =

ifeq ($(HOST_ARCH),ia32e)
    ifeq ($(TARGET_OS), l)
        ifeq ($(KIT),1)
            TESTS = $(TEST_ROOTS:%=%.test)
            CROSS_COMPILE = compile_32_64
            LIN_APPS = $(APPS)        
        endif
     endif
    ifeq ($(TARGET_OS), w)
        TESTS = $(TEST_ROOTS:%=%.test)
        CROSS_COMPILE = compile_32_64
        WIN_APPS = $(APPS) $(OBJDIR)w_early_termination $(OBJDIR)win_launcher_process $(OBJDIR)win_launcher_debugged_process
        ifeq ($(CC),icl)
            MAKE_PREFIX_X86 = cmd /c compiler-env.bat icl_x86
            MAKE_PREFIX_AMD64 = cmd /c compiler-env.bat icl_amd64
        else
            MAKE_PREFIX_X86 = cmd /c compiler-env.bat x86
            MAKE_PREFIX_AMD64 = cmd /c compiler-env.bat amd64      
        endif
    endif
endif

############
#build rules

all: $(CROSS_COMPILE)

$(OBJDIR):
	mkdir -p $(OBJDIR)

compile_32_64:
	$(MAKE_PREFIX_X86) make TARGET=ia32 $(TARGET_OS)_tools_and_apps
	$(MAKE_PREFIX_AMD64) make TARGET=ia32e $(TARGET_OS)_tools_and_apps
	
w_tools_and_apps: $(OBJDIR) $(TOOLS) $(WIN_APPS)

l_tools_and_apps: $(OBJDIR) $(TOOLS) $(LIN_APPS)

DIR_WITH_SPACE1 = "Dir with space 1"
DIR_WITH_SPACE2 = "Dir with space 2"

###########	
#test rules

test: $(CROSS_COMPILE) $(TESTS)

cross_32_64_32.test: %.test: $(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/$(PARENT_PROCESS) ./$(OBJDIR_intel64)/$(CHILD_PROCESS) ./$(OBJDIR_ia32)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	rm $*.failed  $*.out
	
cross_64_32_64.test: %.test: $(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_intel64)/$(PARENT_PROCESS) ./$(OBJDIR_ia32)/$(CHILD_PROCESS) ./$(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	rm $*.failed $*.out

cross_32_64_32_probe.test: %.test: $(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/$(PARENT_PROCESS) ./$(OBJDIR_intel64)/$(CHILD_PROCESS) ./$(OBJDIR_ia32)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	rm $*.failed $*.out
	
cross_64_64_32_log.test: %.test: $(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -xyzzy -log_server 1 -logfile pin.log -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_intel64)/$(PARENT_PROCESS) ./$(OBJDIR_intel64)/$(CHILD_PROCESS) ./$(OBJDIR_ia32)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	#verify 2 files are created (for 2 servers)
	test `rm -vf pin.log.* | wc -l` -eq "2" 
	rm $*.failed $*.out	

cross_32_32_64_probe_log.test: %.test: $(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -xyzzy -log_server 1 -logfile "pin probe.log" -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/$(PARENT_PROCESS) ./$(OBJDIR_ia32)/$(CHILD_PROCESS) ./$(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	#verify 2 files are created (for 2 servers)
	test `rm -vf "pin probe.log".* | wc -l` -eq "2" 
	rm $*.failed $*.out
	
sigchld_cross.test: %.test: $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -- ./$(OBJDIR_ia32)/sigchld_app ./$(OBJDIR_intel64)/sigchld_app
	$(PIN_DIFF) sigchld_app.out sigchld_app.reference
	rm $*.failed sigchld_app.out
	
cross_32_64_32_sym.test: %.test: $(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -xyzzy -log_server 1 -logfile pinsm.log -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) -xyzzy -log_server 1 -logfile pinsm.log -- ./$(OBJDIR_ia32)/$(PARENT_PROCESS) ./$(OBJDIR_intel64)/$(CHILD_PROCESS) ./$(OBJDIR_ia32)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	#verify 2 files are created (for 2 servers)
	test `rm -vf pinsm.log.* | wc -l` -eq "2"
	rm $*.failed  $*.out

cross_64_32_64_sym.test: %.test: $(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -xyzzy -log_server 1 -logfile pinsm.log -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX)  -t ./$(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) -xyzzy -log_server 1 -logfile pinsm.log -- ./$(OBJDIR_intel64)/$(PARENT_PROCESS) ./$(OBJDIR_ia32)/$(CHILD_PROCESS) ./$(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	#verify 2 files are created (for 2 servers)
	test `rm -vf pinsm.log.* | wc -l` -eq "2"
	rm $*.failed  $*.out

cross_32_64_32_probe_sym.test: %.test: $(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -xyzzy -log_server 1 -logfile pinsmp.log -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) -xyzzy -log_server 1 -logfile pinsmp.log -- ./$(OBJDIR_ia32)/$(PARENT_PROCESS) ./$(OBJDIR_intel64)/$(CHILD_PROCESS) ./$(OBJDIR_ia32)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	#verify 2 files are created (for 2 servers)
	test `rm -vf pinsmp.log.* | wc -l` -eq "2"
	rm $*.failed  $*.out

cross_64_32_64_probe_sym.test: %.test: $(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -xyzzy -log_server 1 -logfile pinsmp.log -follow_execv -t64 ./$(OBJDIR_intel64)/follow_child_tool_sym$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/follow_child_tool_sym$(PINTOOL_SUFFIX) -xyzzy -log_server 1 -logfile pinsmp.log -- ./$(OBJDIR_intel64)/$(PARENT_PROCESS) ./$(OBJDIR_ia32)/$(CHILD_PROCESS) ./$(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	#verify 2 files are created (for 2 servers)
	test `rm -vf pinsmp.log.* | wc -l` -eq "2"
	rm $*.failed  $*.out

environment.test: %.test: $(CROSS_COMPILE) %.tested %.failed
	export LD_LIBRARY_PATH="APP_PATH:"$(LD_LIBRARY_PATH); $(PIN) -follow_execv -xyzzy -mesgon log_injector  -- ./$(OBJDIR_intel64)/$(PARENT_PROCESS) ./$(OBJDIR_ia32)/$(CHILD_PROCESS) ./$(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	grep "LD_LIBRARY_PATH for injector" pin.log|grep $(TARGET_LONG)/runtime >>$*.out
	grep "LD_LIBRARY_PATH for pin vm"   pin.log|grep $(TARGET_LONG)/runtime/glibc >>$*.out
	grep "LD_LIBRARY_PATH for application" pin.log|grep APP_PATH >>$*.out
	rm $*.failed $*.out pin.log

space.test: %.test: $(CROSS_COMPILE) %.tested %.failed
	mkdir -p $(DIR_WITH_SPACE1)
	$(PIN) -follow_execv -t64 $(DIR_WITH_SPACE1)/../$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t $(DIR_WITH_SPACE1)/../$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- $(OBJDIR_intel64)/$(PARENT_PROCESS) $(OBJDIR_ia32)/$(CHILD_PROCESS) $(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	rm -r $*.failed $*.out $(DIR_WITH_SPACE1)
	
## space_probe fails if LD_LIBRARY_PATH is set incorrectly
space_probe.test: %.test: $(CROSS_COMPILE) %.tested %.failed
	mkdir -p $(DIR_WITH_SPACE2)
	$(PIN) -probe -follow_execv -t64 $(DIR_WITH_SPACE2)/../$(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t $(DIR_WITH_SPACE2)/../$(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- $(OBJDIR_intel64)/$(PARENT_PROCESS) $(OBJDIR_ia32)/$(CHILD_PROCESS) $(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	rm -r $*.failed $*.out $(DIR_WITH_SPACE2)
	
w_space.test: %.test: $(CROSS_COMPILE) %.tested %.failed
	mkdir -p "bin ia 32"
	mkdir -p "bin intel 64"
	cp -r $(dir $(PIN_EXE_32))/* "bin ia 32"
	cp -r $(dir $(PIN_EXE_64))/* "bin intel 64"
	cp -f $(OBJDIR_ia32)/$(CHILD_PROCESS)  bin.exe	
	"bin ia 32/pin.exe" -p32 "bin ia 32/pin.exe" -p64 "bin intel 64/pin.exe" -follow_execv -t64 $(OBJDIR_intel64)/follow_child_tool$(PINTOOL_SUFFIX) -t $(OBJDIR_ia32)/follow_child_tool$(PINTOOL_SUFFIX) -- $(OBJDIR_intel64)/$(PARENT_PROCESS) $(OBJDIR_ia32)/$(CHILD_PROCESS) $(OBJDIR_intel64)/$(GRAND_CHILD_PROCESS) "param1 param2" param3 >$*.out
	test `grep "At follow child callback" $*.out | wc -l` -eq "2"
	rm -r $*.failed $*.out "bin ia 32" "bin intel 64" bin.exe	

w_cross_64_32_early_termination.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_intel64)/win_launcher_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out
	
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_intel64)/win_launcher_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	
	
w_cross_64_32_early_termination_probed.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_intel64)/win_launcher_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out	
		
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_intel64)/win_launcher_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	

w_cross_32_64_early_termination.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/win_launcher_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out
	
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_ia32)/win_launcher_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	
	
w_cross_32_64_early_termination_probed.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/win_launcher_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out
	
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_ia32)/win_launcher_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out
	
w_cross_64_32_early_termination_debugged.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_intel64)/win_launcher_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out
	
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_intel64)/win_launcher_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	
	
w_cross_64_32_early_termination_probed_debugged.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_intel64)/win_launcher_debugged_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out	
		
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_intel64)/win_launcher_debugged_process ./$(OBJDIR_ia32)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"	
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	

w_cross_32_64_early_termination_debugged.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/win_launcher_debugged_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out
	
	$(PIN) -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_ia32)/win_launcher_debugged_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	
	
w_cross_32_64_early_termination_probed_debugged.test: %.test:  $(CROSS_COMPILE) %.tested %.failed
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -- ./$(OBJDIR_ia32)/win_launcher_debugged_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.out
	
	$(PIN) -probe -follow_execv -t64 ./$(OBJDIR_intel64)/w_follow_child_tool$(PINTOOL_SUFFIX) -t ./$(OBJDIR_ia32)/w_follow_child_tool$(PINTOOL_SUFFIX) -load_system_dlls 1 -- ./$(OBJDIR_ia32)/win_launcher_debugged_process ./$(OBJDIR_intel64)/w_early_termination  >$*.out
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "2"
	test `grep "At follow child callback" $*.out | wc -l` -eq "1"
	grep "Terminating process in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out	

############
#clean rules	
clean:
	rm -rf $(OBJDIR_intel64) $(OBJDIR_ia32) *.out *.tested *.failed *.obj *.log *.log.*

####################################
#TOOLS and APPS specific build rules

$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) : $(OBJDIR)%.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(LIN_APPS) : $(OBJDIR)% : $(OBJDIR)%.o
	${CXX} $(APP_CXXFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $<

$(WIN_APPS) :  $(OBJDIR)% : %.cpp
	$(CXX) $(APP_CXXFLAGS2) $(DBG) ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)
	
$(OBJDIR)w_early_termination: w_early_termination.cpp $(OBJDIR)w_terminate_process_dll.dll
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)w_terminate_process_dll.lib
	
$(OBJDIR)w_terminate_process_dll.dll: w_terminate_process_dll.cpp
	$(CXX) $(APP_CXXFLAGS2)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS) /dll	
