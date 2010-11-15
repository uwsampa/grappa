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

ifeq ($(TARGET_OS),w)
    OS_API=windows
else
    OS_API=unix
endif

ifneq ($(ENABLE_VS), 1)
    VS_FLAG = -xyzzy -virtual_segments 1
else
    VS_FLAG =
endif
## Set up tool roots

APPS = 

TOOL_ROOTS = returnreg args stackalign syscall partialinline memalign fini nops control_init line realloc inlined-stack-arg \
 earlymalloc inscount2_fornoinline str2int str2flt thenpredicated checkerrorfile posix_memalign 
ifneq ($(TARGET),ipf)
    TOOL_ROOTS += xmmtest 
    APPS += xmmapp$(EXEEXT)
endif
STATIC_TOOL_ROOTS =

TOOL_ROOTS += incebx args_err write_user_error claim_regs args_lineno check_legal_callbacks

# don't test routine replacement when virtual segments flag is enabled
#ifneq ($(ENABLE_VS),1)
#    TOOL_ROOTS += jitmalloctrace replace_free
#endif

# don't test these on windows.
ifneq ($(TARGET_OS),w)
    TOOL_ROOTS += soload mmap mt malmalloc change_syscall
    STATIC_TOOL_ROOTS += statica_locktest
endif

ifeq ($(TARGET_OS),w)
    TOOL_ROOTS += thread_count thread_count2 stracewin_ia32 exception_monitor apc_monitor debugservice_monitor segmented_ea_verifier image_entry mtflush
    TOOL_ROOTS += internal_exception_handler  image_load
    TOOL_ROOTS += datasym sp_argument win_syscallenum img_open_win 
    ifeq ($(TARGET),ia32)
        TOOL_ROOTS += ea_verifier_addr16
    endif
endif

ifeq ($(TARGET_OS),l)
    ifneq ($(TARGET),ipf)
        TOOL_ROOTS += intrin internal_exception_handler_lin
        TOOL_ROOTS += checkexecuting ea_verifier_bt
        TOOL_ROOTS += elf_symbols 
        APPS += internal_exception_handler_app_lin
        # Definition for testing pause_tool.
        # This test runs only on Linux and has no support to test
        # ia32 on Intel64 systems (requires the cross version of GDB)
        GDB = /usr/bin/gdb
        ifeq (${KIT},0)
            PINBIN=$(PIN_EXE)
        else
            PINBIN=$(PIN_KIT)/$(TARGET_LONG)/bin/pinbin
        endif
        #ifeq ($(TARGET),$(HOST_ARCH))
        #    TOOL_ROOTS += pause_tool
        #endif

    endif
    TOOL_ROOTS += soloadrange mtflush tool_malloc
    APPS += mt_app
endif

ifneq ($(TARGET),ipf)
   # Mantis 1944. This test shows a real bug on IPF, so we skip it for now.
   TOOL_ROOTS += checkinline 
   # gcc asm in test
   ifeq ($(TARGET_OS),l)
       TOOL_ROOTS += int3del
   endif
endif

# safecopy won't work until signals are emulated on Mac and FreeBSD
ifneq ($(TARGET_OS),m)
    TOOL_ROOTS += safecopy
endif

ifneq ($(TARGET_OS),m)
ifneq ($(TARGET),ipf)
    ifeq ($(TARGET_OS),l)
        # there is a bug in Linux kernels prior to 2.6.9 that cause this test to fail
        # the only supported OS with such a kernel is SLES9 with kernel 2.6.5
        SLES9VER = 2.6.5
        ifneq ($(findstring $(SLES9VER),$(OSREL)),$(SLES9VER))
            TOOL_ROOTS += safecopy_mxcsr
        endif
    else
        TOOL_ROOTS += safecopy_mxcsr
    endif
endif
endif

ifneq ($(TARGET),ipf)
  TOOL_ROOTS += operandtool returnflags leave emu_stack emu_jumps inlined-stack-arg1  
endif

ifeq ($(TARGET_OS),w)
    ifeq ($(TARGET),ia32)
       TOOL_ROOTS += swizzle_seg 
       APPS += swizzle_seg_app_$(TARGET_OS)$(EXEEXT)
    else
       TOOL_ROOTS += test_ip_access 
    endif
endif

ifeq ($(TARGET_OS),l)
    TOOL_ROOTS += undecorate 
    APPS += undecorate_app
    ifneq ($(TARGET),ipf)
        TOOL_ROOTS += swizzle2 swizzle3 swizzle_seg 
        APPS += swizzle_seg_app_$(TARGET_OS)$(EXEEXT)
    endif
endif


ifeq ($(TARGET),ia32)
    TOOL_ROOTS += rewritememop rewritememop1 inlinecall repcmpsz_tool checkreps 
    TOOL_ROOTS += toolfetch_sub jitmalloctrace replace_free  executeat_callback 
    TOOL_ROOTS += sse-ref inlined-stack-arg-noopt inlined-stack-arg1-noopt 
    ifneq ($(TARGET_OS),w)
        TOOL_ROOTS += branch_target_addr
    endif
    ifeq ($(TARGET_OS),l)
        TOOL_ROOTS += swizzle5 executeat_lock popea_verifier fp_replace fp_insert create_rtn fp_args aoti_mutation_tool special
        TOOL_ROOTS += context_state_apis 
        APPS += create_rtn_app$(EXEEXT) aoti_mutation_target$(EXEEXT)
    endif
endif

ifeq ($(TARGET),ia32e)
    TOOL_ROOTS += jitmalloctrace replace_free  executeat_callback str2int64 
   TOOL_ROOTS += sse-ref inlined-stack-arg-noopt inlined-stack-arg1-noopt 
    ifeq ($(TARGET_OS),l)
        TOOL_ROOTS += special executeat_lock rewritememop1 popea_verifier fp_replace fp_insert fp_args
    endif
endif

ifeq ($(TARGET),ipf)
    TOOL_ROOTS += predinlinetest retcnt toolfetch_sub str2int64
else
    TOOL_ROOTS += sse-unaligned-class sse-unaligned-class2 set_fp_context_xmm_regs
    APPS += set_fp_context_xmm_regs_app$(EXEEXT)
endif

# Tests for the knobs system
TOOL_ROOTS += knob_writeonce knob_duplicate_name
TESTS_WITHOUT_TOOLS = knob_help knob_version
ifeq ($(TARGET_OS),l)
    TESTS_WITHOUT_TOOLS += knob_version_long_lin
endif


# Generic
TOOL_ROOTS += align_access

# every tool should be tested
TEST_TOOLS = $(TOOL_ROOTS)
STATIC_TEST_TOOLS = $(STATIC_TOOL_ROOTS)

#
# tests that have no tools, or require special building of the tool
#

# Generic
TEST_TOOLS += resize memory_reuse 

# ISA dependent
IA32_TEST_TOOLS = mtstatic kthread htab smc_sse smc_ia32 smc_mt 
ifeq ($(TARGET),ia32)
    ifeq ($(TARGET_OS),l)
      TEST_TOOLS +=   int3del reservemem callsp_ia32 far_ret smallpush rwm_smallpush \
	rwm_farret rwm_pushtest rwm1_str \
	rwm_farret1 rwm_pushtest1 rwm1_str1 rwm_smallpush1 \
	rwm_farret2 rwm_pushtest2 rwm1_str2 rwm_smallpush2 pusha_popa 
    APPS += pushtest$(EXEEXT) far pusha_popa$(EXEEXT)
    endif
    TEST_TOOLS += $(IA32_TEST_TOOLS) rwm_pushapopa rwm_pushapopa1 rwm_pushapopa2
    APPS += smcapp_ia32 smcapp_mt sse set_fp_context_xmm_regs_app$(EXEEXT) 
    ifeq ($(PROBE), 1)
      TEST_TOOLS += create_rtn_probed
    endif
endif
ifeq ($(TARGET),ia32e)
   ifeq ($(TARGET_OS),l)
      TEST_TOOLS +=   int3del reservemem64 callsp_intel64 smallpush \
	rwm_smallpush rwm_smallpush1 rwm_smallpush2
   endif
   TEST_TOOLS += $(IA32_TEST_TOOLS) spalign nosahflahf 
endif
ifeq ($(TARGET),ipf)
    # Add 'attachnat' when kernel is fixed
    TEST_TOOLS += kthread htab smc_ipf
endif

CALLAPP_IA32 = callapp1 callapp2 callapp10 callapp1i callapp10i 
CALLAPP_IA32 += callapp5 callapp6 callapp7 callapp8 callapp9 callapp14
CALLAPP_IA32 += callapp12 replace_malloc_inst thread_callback
CALLAPP_IA32E = $(CALLAPP_IA32) callapp3 callapp4 callapp13
CALLAPP_IA32_LINUX = callapp0 callapp0i callapp15
CALLAPP_IA32E_LINUX = callapp15
CALLAPP_IA32_WINDOWS = callappfast10 callappstd10

# OS dependent
ifeq ($(TARGET_OS),l)
    TEST_TOOLS += pipe waitpidbug cancel earlymallocexe thread_usestack 
    TOOL_ROOTS += earlyout mt_tool
    EXETOOLS_ROOTS = earlymallocexe
    ifeq ($(TARGET),ia32)
        TOOL_ROOTS += strace_ia32 reptool short_name context dlclose
        TOOL_ROOTS += $(CALLAPP_IA32) $(CALLAPP_IA32_LINUX) attach funcarg_ep flag_ac_tool 
        TEST_TOOLS += badpath badfile smc_except attach checkexecuting
        APPS += attach_app funcarg_ep_app smcapp_except
    endif
    ifeq ($(TARGET),ia32e)
        TOOL_ROOTS += strace_ia32 reptool short_name context attach 
        TOOL_ROOTS += $(CALLAPP_IA32E) $(CALLAPP_IA32E_LINUX) funcarg_ep flag_ac_tool 
        TEST_TOOLS += insertand clobber clobber_xmm smc_except attach 
        APPS += attach_app funcarg_ep_app smcapp_except
        TEST_TOOLS += checkexecuting
    endif
    ifeq ($(TARGET),ipf)
        TOOL_ROOTS += strace_ipf short_name
    endif

    MT = -mt 0
    SIMPLEFOO=simplefoo
    SIMPLEFOO1=simplefoo1
    SIMPLEFOO2=simplefoo2
    SIMPLEFOO3=simplefoo3
    SIMPLEFOO4=simplefoo4
    SIMPLEFOO6=simplefoo6
    SIMPLEFOO7=simplefoo7
    SIMPLEFOO8=simplefoo8
    SIMPLEFOO9=simplefoo9
    SIMPLEFOO10=simplefoo10
    SIMPLEFOO12=simplefoo12
    SIMPLEFOO13=simplefoo13
    SIMPLEFOO14=simplefoo14
    SIMPLEFOO1i=simplefoo1i
    SIMPLEFOO10i=simplefoo10i
    LITTLE_MALLOC=little_malloc
    THREADTEST=thread_wait
    SET_FP_CONTEXT_XMM_REGS_APP=set_fp_context_xmm_regs_app
    INNER=inner
endif

# We want to test using a 32 bit with 64 bit pin
#ifeq ($(TARGET),ia32e)
#    ifeq ($(TARGET_OS),l)
#    TEST_TOOLS += hello32
#    endif
#endif

ifeq ($(TARGET_OS),w)
  TEST_TOOLS += secname_tool
    ifeq ($(TARGET_COMPILER),ms)
        TOOL_ROOTS += mt malmalloc funreplace_alert syscall_std teb context
        ifeq ($(TARGET), ia32e)
            TOOL_ROOTS += $(CALLAPP_IA32E) set_fp_context_xmm_regs
        else
            TOOL_ROOTS += $(CALLAPP_IA32) set_fp_context_xmm_regs
                        TEST_TOOLS += inlined-stack-arg2
            TEST_TOOLS += jcx_addr16_tool baserel_in_probe_tool
        endif
        TOOL_ROOTS += suspend_win guard_page raise_exception sw_interrupt win_callback
        TEST_TOOLS += suspend_context_win 
        WIN_SIGNAL_TESTS = thread_count_thread_creation win_queue_apc win_divide_by_zero_exception \
                           win_cpp_exception
        ifeq ($(TARGET), ia32e)
            WIN_SIGNAL_TESTS += win_divide_by_zero_exception_for_iretq_order
        endif
        TEST_TOOLS += ${WIN_SIGNAL_TESTS}
        TEST_TOOLS += win_debug_service win_load_library win_code_on_reused_memory win_multi_dll_pintool
        TEST_TOOLS += load_dummy_dll_tool smc_except rebase_dll_tool exports_only_tool win_exception_context
        TEST_TOOLS += flag_ac_win_tool win_early_injection1 win_early_injection1_probed thread_creation_late_injection
        ifneq ($(ICC),)
            TEST_TOOLS += win_mxcsr 
        endif 
        ifeq ($(ICC),)
            TEST_TOOLS += win_hello_c_sharp win_early_injection2 win_early_injection2_probed
        endif
        ifneq ($(TARGET_COMPILER),gnu)
            ifeq ($(TARGET),ia32)
                TEST_TOOLS += df_test_tool1 df_test_tool2 df_test_tool3 df_test_tool4 df_test_tool5 analysis_flag_overwrite_tool1 analysis_flag_overwrite_tool2 analysis_flag_overwrite_tool3
                TEST_TOOLS += analysis_flag_overwrite_tool1 analysis_flag_overwrite_tool2 analysis_flag_overwrite_tool3 reg_operands_test_tool 
                ifeq ($(ICC),)
                    # ICC incorrectly compiles inline assembly code in this test 
                    TEST_TOOLS += smc_bbl
                endif
            endif
        endif
    endif

    SIMPLEFOO1=winfoo1
    SIMPLEFOO2=winfoo2
    SIMPLEFOO3=winfoo3
    SIMPLEFOO4=winfoo4
    SIMPLEFOO5=winfoo5
    SIMPLEFOO6=winfoo6
    SIMPLEFOO7=winfoo7
    SIMPLEFOO8=winfoo8
    SIMPLEFOO9=winfoo9
    SIMPLEFOO10=winfoo10
    SIMPLEFOO12=winfoo12
    SIMPLEFOO13=winfoo13
    SIMPLEFOO14=winfoo14
    SIMPLEFOO1i=winfoo1i
    SIMPLEFOO10i=winfoo10i
    LITTLE_MALLOC=little_malloc.exe
    THREADTEST=threadtestwin.exe
    SET_FP_CONTEXT_XMM_REGS_APP=set_fp_context_xmm_regs_app.exe
    INNER=inner.exe
endif

TOOL_ROOTS += reg_inst_gx

ifeq ($(TARGET_OS),m)
    #exclude mtstatic, sigmaskstatic
    TOOL_ROOTS = mt malmalloc args stackalign line \
                 returnreg returnflags partialinline syscall branch_target_addr memalign 
    ifeq ($(TARGET),ia32)
        TOOL_ROOTS += strace_ia32 context
    endif
    TEST_TOOLS = sigmask-mac resize kthread htab sse-ref $(TOOL_ROOTS)
endif

ifeq ($(TARGET_OS),m)
    HELLO=hello-mac
    DLTEST=dltest-mac
    MTFLUSHAPP=mtflushapp_unix
else
    ifeq ($(TARGET_OS),w)
        HELLO=hello-ms-$(TARGET)
        MTFLUSHAPP=mtflushapp_win.exe
    else
        HELLO=hello
        DLTEST=dltest
        DLTEST2=dltest2
        MTFLUSHAPP=mtflushapp_unix
    endif
endif

ifeq ($(TARGET_OS),b)
    TOOL_ROOTS += swizzle2 special earlyout strace_ia32 reptool short_name context int3del executeat_lock
    TOOL_ROOTS += swizzle_seg $(CALLAPP_IA32E) undecorate
    APPS += swizzle_seg_app_$(TARGET_OS)$(EXEEXT)
    TEST_TOOLS += earlymallocexe pipe insertand clobber thread_usestack

    SIMPLEFOO=simplefoo
    SIMPLEFOO1=simplefoo1
    SIMPLEFOO2=simplefoo2
    SIMPLEFOO3=simplefoo3
    SIMPLEFOO4=simplefoo4
    SIMPLEFOO6=simplefoo6
    SIMPLEFOO7=simplefoo7
    SIMPLEFOO8=simplefoo8
    SIMPLEFOO9=simplefoo9
    SIMPLEFOO10=simplefoo10
    SIMPLEFOO12=simplefoo12
    SIMPLEFOO13=simplefoo13
    SIMPLEFOO14=simplefoo14
    SIMPLEFOO1i=simplefoo1i
    SIMPLEFOO10i=simplefoo10i
    LITTLE_MALLOC=little_malloc
    THREADTEST=thread_wait
    SET_FP_CONTEXT_XMM_REGS_APP=set_fp_context_xmm_regs_app
    INNER=inner
endif

ONEPROG=oneprog

# if you want to build, but not test a tool, add it to TOOL_ROOTS here
# TOOL_ROOTS += traceall

TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
STATIC_TOOLS = $(STATIC_TOOL_ROOTS:%=$(OBJDIR)%$(SATOOL_SUFFIX))
APPS  += $(DLTEST) $(DLTEST2) $(HELLO) $(MTFLUSHAPP) mmapapp pipeapp
ifeq ($(TARGET),ipf)
    APPS += smcapp_ipf attachnat
endif
ifeq ($(TARGET),ia32)
    APPS += stringtest
    ifeq ($(TARGET_OS),w)
        CMPXCHG8_ASM_OBJ = cmpxchg8b_ms.obj
        CMPXCHG8_WITH_EXPLICIT_EBX_ASM_OBJ = cmpxchg8b_with_explicit_ebx_ms.obj
        FLAG_SPILL_FILL_APP_ASM_OBJ = flag_spill_fill_app_ms.obj
        FLAG_SPILL_FILL_TOOL1_ASM_OBJ = flag_spill_fill_tool1_ms.obj
        ANALYSIS_FLAG_OVERWRITE_APP_ASM_OBJ = analysis_flag_overwrite_app_ms.obj
        ANALYSIS_FLAG_OVERWRITE_TOOL1_ASM_OBJ = analysis_flag_overwrite_tool1_ms.obj
        ANALYSIS_FLAG_OVERWRITE_TOOL2_ASM_OBJ = analysis_flag_overwrite_tool2_ms.obj
        ANALYSIS_FLAG_OVERWRITE_TOOL3_ASM_OBJ = analysis_flag_overwrite_tool3_ms.obj
        FLAG_AC_APP_ASM_OBJ = flag_ac_app_ms.obj
        DF_TEST_APP_ASM_OBJ = df_test_app_ms.obj
        DF_TEST_TOOL1_ASM_OBJ = df_test_tool1_ms.obj
        DF_TEST_TOOL2_ASM_OBJ = df_test_tool2_ms.obj
        DF_TEST_TOOL3_ASM_OBJ = df_test_tool3_ms.obj
        DF_TEST_TOOL4_ASM_OBJ = df_test_tool4_ms.obj
        DF_TEST_TOOL5_ASM_OBJ = df_test_tool5_ms.obj
        REG_OPERANDS_TEST_APP_ASM_OBJ = reg_operands_test_app_ms.obj
        SEG_OVERRIDE_APP1_ASM_OBJ = seg_override_app1_ms.obj
        INLINED_STACK_ARG2_ASM_OBJ = inlined-stack-arg2_ms.obj
        MXCSR_ASM_OBJ = mxcsr_ia32.obj
        SUPPORTS_AVX_ASM_OBJ = supports_avx_ia32.obj
        INLINE_OPT_TEST_ASM_OBJ = inline_opt_test_ia32.obj
        INLINE_OPT_TEST_DF_ASM_OBJ = inline_opt_test_df_ia32.obj
        INLINE_OPT_TEST_APP_ASM_OBJ = inline_opt_test_app_ia32.obj
        INLINE_OPT_TEST_DF_APP_ASM_OBJ = inline_opt_test_df_app_ia32.obj
    else
        APPS += doint_ia32
        CMPXCHG8_ASM_OBJ = cmpxchg8b_gnu.o
        FLAG_AC_APP_ASM_LIN_SRC = flag_ac_app_u.s
        MXCSR_ASM_OBJ = mxcsr_ia32.o
        SUPPORTS_AVX_ASM_OBJ = supports_avx_ia32.o
        INLINE_OPT_TEST_ASM_OBJ = inline_opt_test_ia32.o
        INLINE_OPT_TEST_APP_ASM_OBJ = inline_opt_test_app_ia32.o
    endif
endif
ifeq ($(TARGET),ia32e)
    ifeq ($(TARGET_OS),w)
        FLAG_AC_APP_ASM_OBJ = flag_ac_app_m_intel64.obj
        MXCSR_ASM_OBJ = mxcsr_intel64.obj
        SUPPORTS_AVX_ASM_OBJ = supports_avx_intel64.obj
        INLINE_OPT_TEST_ASM_OBJ = inline_opt_test_intel64.obj
        INLINE_OPT_TEST_APP_ASM_OBJ = inline_opt_test_app_intel64.obj
    else
        FLAG_AC_APP_ASM_LIN_SRC = flag_ac_app_u_intel64.s
        MXCSR_ASM_OBJ = mxcsr_intel64.o
        SUPPORTS_AVX_ASM_OBJ = supports_avx_intel64.o
        INLINE_OPT_TEST_ASM_OBJ = inline_opt_test_intel64.o
        INLINE_OPT_TEST_APP_ASM_OBJ = inline_opt_test_app_intel64.o
    endif
endif

APPS_BINARY_FILES = $(APPS:%=$(OBJDIR)%)
EXE_TOOLS = $(EXETOOLS_ROOTS:%=$(OBJDIR)%)


ifeq ($(TARGET_OS),w)
all: tools 
else
all: tools $(APPS_BINARY_FILES)
endif
tools: $(OBJDIR) $(TOOLS) $(STATIC_TOOLS) $(EXE_TOOLS)

ifeq ($(TARGET),ia32)
callapps: $(OBJDIR) $(CALLAPP_IA32:%=%.test)
endif
ifeq ($(TARGET),ia32e)
callapps: $(OBJDIR) $(CALLAPP_IA32E:%=%.test)
endif


## sanity

SANITY_TESTS = args.test line.test replace_free.test args_err.test write_user_error.test 
SANITY_TESTS += args_lineno.test


ifneq ($(TARGET_OS),m)
    SANITY_TESTS += safecopy.test
endif


ifeq ($(TARGET_OS),w)
    SANITY_TESTS += sp_argument.test win_syscallenum.test
    ifeq ($(TARGET_COMPILER),ms)
        SANITY_TESTS += funreplace_alert.test syscall_std.test teb.test segmented_ea_verifier.test 
        SANITY_TESTS += smc_except.test win_exception_context.test suspend_win.test suspend_context_win.test
        SANITY_TESTS += guard_page.test raise_exception.test sw_interrupt.test internal_thread.test
        SANITY_TESTS += win_callback.test
        SEGMENTED_EA_VERIFIER_ASM_OBJ = $(OBJDIR)segmented_ea_verifier_win1_ms.obj
        ifeq ($(TARGET),ia32)
            SANITY_TESTS += ea_verifier_addr16.test
            ifeq ($(ICC),)
                # ICC incorrectly compiles inline assembly code in this test 
                SANITY_TESTS += smc_bbl.test
            endif
        endif
    endif
endif

ifeq ($(TARGET_OS),l)
    SANITY_TESTS += internal_thread.test
    ifneq ($(TARGET),ipf)
        SANITY_TESTS += smc_except.test
        SEGMENTED_EA_VERIFIER_ASM_OBJ = $(OBJDIR)segmented_ea_verifier_lin_$(TARGET).o
        EA_VERIFIER_ADDR16_ASM_OBJ = $(OBJDIR)ea_verifier_addr16_lin$(TARGET).o
    endif
endif


ifeq ($(TARGET),ia32e)
    SANITY_TESTS += sse-ref.test smc_sse.test smc_ia32.test smc_mt.test 
    ifeq ($(TARGET_OS),l)
       SANITY_TESTS += int3.test
    endif
    ifeq ($(TARGET_COMPILER),ms)
        SET_XMM_SCRATCH_REGS_OBJ = $(OBJDIR)set_xmm_scratches_asmIntel64.obj
        SSE-REF_ASM_OBJ = $(OBJDIR)sse-ref_ia32e.obj
        SP_ALIGN_ASM_OBJ = $(OBJDIR)spalign_asm_ia32e_ms.obj
        TEST_IP_ACCESS_ASM_OBJ = $(OBJDIR)test_ip_access_app_ia32e.obj
        SEGMENTED_EA_VERIFIER_ASM_OBJ = $(OBJDIR)segmented_ea_verifier_win1_ia32e_ms.obj
        EA_VERIFIER_ADDR16_ASM_OBJ = $(OBJDIR)ea_verifier_addr16_ia32e_ms.obj
        LINK_OPTION     = /link
    else
        SP_ALIGN_ASM_OBJ = $(OBJDIR)spalign_asm_ia32e_gcc.o
    endif
endif

ifeq ($(TARGET),ia32)
    SANITY_TESTS += sse-ref.test smc_sse.test smc_ia32.test smc_mt.test 
    ifeq ($(TARGET_OS),l)
       SANITY_TESTS += int3.test
    endif
    ifeq ($(TARGET_COMPILER),ms)
        SSE-REF_ASM_OBJ = $(OBJDIR)sse-ref_ia32.obj
        SWIZZLETEST_ASM_OBJ = $(OBJDIR)swizzlealloc_ia32.obj
        EA_VERIFIER_ADDR16_ASM_OBJ = $(OBJDIR)ea_verifier_addr16_ms.obj
    endif
endif



TESTS_TO_RUN = $(TEST_TOOLS:%=%.test) $(STATIC_TEST_TOOLS:%=%.test) $(TESTS_WITHOUT_TOOLS:%=%.test)

ifeq ($(TARGET),ia32e)
    ifneq ($(TARGET_OS),w)
        TESTS_TO_RUN += big_bss.test
    endif
    ifneq ($(TARGET_OS),m)
	TESTS_TO_RUN += checkinline_nocmov.test spalign_no_inline_bridge.test
    endif
endif

ifeq ($(TARGET),ia32)
    ifneq ($(TARGET_OS),m)
	TESTS_TO_RUN += checkinline_nocmov.test
    endif
    ifeq ($(TARGET_COMPILER),ms)
        TESTS_TO_RUN +=  df_test1_inline.test df_test1_noinline.test df_test1_noinline_bridge.test
        TESTS_TO_RUN += df_test2_inline.test df_test2_noinline.test df_test2_noinline_bridge.test
        TESTS_TO_RUN += df_test3_inline.test df_test3_noinline.test df_test3_noinline_bridge.test 
        TESTS_TO_RUN += df_test4_inline.test df_test4_noinline.test df_test4_noinline_bridge.test 
        TESTS_TO_RUN += df_test5_inline.test df_test5_noinline.test df_test5_noinline_bridge.test 
        TESTS_TO_RUN += analysis_flag_overwrite_test1_inline.test analysis_flag_overwrite_test1_noinline.test analysis_flag_overwrite_test1_noinline_bridge.test
        TESTS_TO_RUN += analysis_flag_overwrite_test2_inline.test analysis_flag_overwrite_test2_noinline.test analysis_flag_overwrite_test2_noinline_bridge.test
        TESTS_TO_RUN += analysis_flag_overwrite_test3_inline.test analysis_flag_overwrite_test3_noinline.test analysis_flag_overwrite_test3_noinline_bridge.test
        TESTS_TO_RUN += flag_spill_fill_test1_inline.test flag_spill_fill_test1_noinline.test flag_spill_fill_test1_noinline_bridge.test 
        TESTS_TO_RUN += tstcmpxchg8b_with_explicit_ebx.test reg_operands_test.test  win_exception_detect.test failed_attach_with_error_file.test
        TESTS_TO_RUN += inline_opt_test_df1.test inline_opt_test_df2.test inline_opt_test_df3.test
    endif
endif
ifeq ($(TARGET_COMPILER),ms)
    TESTS_TO_RUN += secure_scl.test
endif

ifneq ($(TARGET),ipf)
ifneq ($(TARGET_OS),m)
	TESTS_TO_RUN += move_then_test_inline_exception.test move_then_test_noinline_exception.test move_then_test_noinline_bridge_exception.test
	TESTS_TO_RUN += move_then_test_inline.test move_then_test_noinline.test move_then_test_noinline_bridge.test
endif
endif

ifeq ($(TARGET_OS),l)
    TESTS_TO_RUN += internal_thread.test
    ifeq ($(TARGET),ia32e)
        TESTS_TO_RUN += flag_ac_noinline_bridge.test flag_ac_noinline.test flag_ac_inline.test rwm_bitops.test \
          unsup_syscall.test
    endif
    ifeq ($(TARGET),ia32)
        TESTS_TO_RUN += flag_ac_noinline_bridge.test flag_ac_noinline.test flag_ac_inline.test rwm_bitops.test
    endif
endif

ifeq ($(TARGET_OS),w)
    ifeq ($(TARGET_COMPILER),ms)
        TESTS_TO_RUN += internal_thread.test
        TESTS_TO_RUN += load_dummy_dll.test load_dummy_dll_probe.test
        TESTS_TO_RUN += rebase_dll.test rebase_dll_probe.test
        TESTS_TO_RUN += exports_only.test exports_only_probe.test
        TESTS_TO_RUN += dbghelp_version.test
        TESTS_TO_RUN += sympath.test symserv_lowmem.test
        TESTS_TO_RUN += flag_ac_win_noinline_bridge.test flag_ac_win_noinline.test flag_ac_win_inline.test
        ifeq ($(TARGET),ia32e)
            TESTS_TO_RUN += x86dll.test
        else
            TESTS_TO_RUN += baserel_in_probe.test
        endif
    endif
endif

ifneq ($(TARGET),ipf)
ifneq ($(TARGET_OS),m)
    TESTS_TO_RUN += inline_opt_test.test
endif
endif

# opt_stack_param_failure_assert is causing an assert on DEBUG
ifeq ($(DEBUG),1)
    OPT_STACK_PARAM_FAILURE_ASSERT = 
else
    ifeq ($(OPT),)
        OPT_STACK_PARAM_FAILURE_ASSERT = 
    else
        OPT_STACK_PARAM_FAILURE_ASSERT = -opt_stack_param_failure_assert 1
    endif
endif

tests-sanity: $(OBJDIR) $(SANITY_TESTS)
# uncomment when mt working on ipf as2.1
#tests-sanity: htab.test mt.test


test: $(OBJDIR) $(TESTS_TO_RUN)

$(OBJDIR):
	mkdir -p $(OBJDIR)

ifeq ($(TARGET),ipf)
predinlinetest.test: $(OBJDIR)predinlinetest$(PINTOOL_SUFFIX) predinlinetest.tested predinlinetest.failed simple.c
	$(CC) -o $(OBJDIR)simple_static -static simple.c
	$(PIN) -t $< -o inline.out -- ./$(OBJDIR)simple_static result1.out
	$(PIN) -xyzzy -inline 0 -t $< -o noinline.out -- ./$(OBJDIR)simple_static result2.out
	$(PIN_CMP) result1.out result2.out
	$(PIN_CMP) inline.out noinline.out
	rm inline.out noinline.out result1.out result2.out predinlinetest.failed
endif

THREAD_LIB=$(OBJDIR)threadlib.$(OBJEXT)
SYS_LIB=$(THREAD_LIB) $(OBJDIR)sys_memory.$(OBJEXT) 
RUN_LIB=$(OBJDIR)runnable.$(OBJEXT) $(OBJDIR)thread_pool.$(OBJEXT)
SMC_LIB=$(RUN_LIB) $(SYS_LIB) $(OBJDIR)smc_util.$(OBJEXT)
SYS_LIB_TOOL=$(OBJDIR)sys_memory_tool.$(OBJEXT)

$(OBJDIR)threadlib.$(OBJEXT): ../threadlib/threadlib_$(OS_API).c ../threadlib/threadlib.h
	$(CC) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 

$(OBJDIR)sys_memory.$(OBJEXT): sys_memory_$(OS_API).c sys_memory.h
	$(CC) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 

$(OBJDIR)sys_memory_tool.$(OBJEXT): sys_memory_$(OS_API).c sys_memory.h
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)runnable.$(OBJEXT): runnable.cpp runnable.h
	$(CXX) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 

$(OBJDIR)thread_pool.$(OBJEXT): thread_pool.cpp thread_pool.h ../threadlib/threadlib.h
	$(CXX) $(APP_CXXFLAGS) ${COPT} ${OUTOPT}$@ $< 

$(OBJDIR)smc_util.$(OBJEXT): smc_util.cpp smc_util.h
	$(CXX) $(APP_CXXFLAGS) ${COPT} $(OPT) ${OUTOPT}$@ $< 

# file rename for hello.b is to avoid the copyright notice from changing line numbers
# gdwarf-2 is the default on newer compilers
line.test : $(OBJDIR)line$(PINTOOL_SUFFIX) line.tested line.failed $(OBJDIR)$(HELLO)
	rm -f pin.log line.pin.log
	$(PIN) -xyzzy -logfile line.pin.log -separate_memory 0 -t $< -- ./$(OBJDIR)$(HELLO)
	$(PIN_DIFF) line.output line.reference
        # Is pin writing a log file?
	[ ! -f line.pin.log ]
	rm line.output line.failed

symserv_lowmem.test: $(OBJDIR)line$(PINTOOL_SUFFIX) symserv_lowmem.tested symserv_lowmem.failed $(OBJDIR)$(HELLO)
	# Enforce debug info garbage collection in symbol server
	$(PIN) -logfile symserv.log -xyzzy -log_server 1 -t $(OBJDIR)line$(PINTOOL_SUFFIX) -logfile symserv.log -xyzzy -log_server 1 -debug_info_max_size 0 -- ./$(OBJDIR)$(HELLO)
	$(PIN_DIFF) line.output line.reference
	grep "SM:: Unloaded" symserv.log.server*
	rm symserv.log.server* line.output symserv_lowmem.failed

sympath.test : $(OBJDIR)line$(PINTOOL_SUFFIX) sympath.tested sympath.failed $(OBJDIR)$(HELLO)
	rm -f pin.log
	mkdir -p sympath$(OBJDIR)
	cp ./$(OBJDIR)$(HELLO) $(OBJDIR)line$(PINTOOL_SUFFIX) sympath$(OBJDIR)
	# Debug info file resides in parent directory and is not searched by dbghelp.dll
	# No line info output is expected
	cd sympath$(OBJDIR); \
	  ../$(PIN) -t ./line$(PINTOOL_SUFFIX) -- ./$(HELLO)
	grep -c -i "line" sympath$(OBJDIR)line.output | grep "0"
	# Parent directory is searched due to -symbol_path knob.
	# Regular line info output is expected
	cd sympath$(OBJDIR); \
	  ../$(PIN) -t ./line$(PINTOOL_SUFFIX) -symbol_path .. -- ./$(HELLO)
	$(PIN_DIFF) sympath$(OBJDIR)line.output line.reference
	rm -r sympath$(OBJDIR)line.output sympath.failed

$(OBJDIR)hello: hello.b
	cp hello.b hello.c
	$(CC) $(APP_CXXFLAGS) -gdwarf-2 -g -O0 -o $(OBJDIR)hello hello.c -static

$(OBJDIR)hello-ms-$(TARGET) : hello-ms.exe-$(TARGET) hello-ms.pdb-$(TARGET)
	cp hello-ms.exe-$(TARGET) $(OBJDIR)hello-ms-$(TARGET).exe
	cp hello-ms.pdb-$(TARGET) $(OBJDIR)hello-ms-$(TARGET).pdb

$(OBJDIR)hello-ms.exe-$(TARGET) $(OBJDIR)hello-ms.pdb-$(TARGET): hello.b
	cp hello.b hello.c
	$(CC) /Zi /Od /MT /Fe$(OBJDIR)hello-ms.exe-$(TARGET) hello.c /D_STATIC_CPPLIB /link /DEBUG /PDB:$(OBJDIR)hello-ms.pdb-$(TARGET)

$(OBJDIR)hello-mac: hello.b
	cp hello.b hello.c
	$(CC) -g -O0 -o $(OBJDIR)hello-mac hello.c

htab.test: $(OBJDIR)htab htab.tested htab.failed
	$(PIN) -- ./$(OBJDIR)htab
	rm htab.failed

$(OBJDIR)htab: htab.c $(THREAD_LIB)
	$(CC) $(APP_CXXFLAGS)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

threadexit.test: $(OBJDIR)threadexit threadexit.tested threadexit.failed
	$(PIN) -- ./$(OBJDIR)threadexit >  $<.out 2>&1
	$(PIN_DIFF) $<.out threadexit.reference
	rm threadexit.failed $<.out

$(OBJDIR)threadexit: threadexit.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)threadexit threadexit.c $(APP_PTHREAD)

thread_usestack.test: $(OBJDIR)thread_usestack thread_usestack.tested thread_usestack.failed
	$(PIN) -- ./$(OBJDIR)thread_usestack >  $<.out 2>&1
	rm thread_usestack.failed $<.out

$(OBJDIR)thread_usestack: thread_usestack.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)thread_usestack thread_usestack.c $(APP_PTHREAD)

checkexecuting.test : $(OBJDIR)checkexecuting$(PINTOOL_SUFFIX) $(OBJDIR)cmovstest checkexecuting.tested checkexecuting.failed
	$(PIN) -t $< -- ./$(OBJDIR)cmovstest >  cmovstest.out 2>&1
	diff -w checkexecuting.out cmovstest.out
	rm checkexecuting.failed checkexecuting.out cmovstest.out

$(OBJDIR)cmovstest: cmovstest.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)cmovstest cmovstest.c 

ea_verifier_bt.test : $(OBJDIR)ea_verifier_bt$(PINTOOL_SUFFIX) $(OBJDIR)btc_app ea_verifier_bt.tested ea_verifier_bt.failed
	$(PIN) -t $< -- ./$(OBJDIR)btc_app >  btc_app.out 2>&1
	rm ea_verifier_bt.failed ea_verifier_bt.out btc_app.out

mt.test : $(OBJDIR)mt$(PINTOOL_SUFFIX) $(OBJDIR)thread mt.tested mt.failed
	$(PIN) -t $< -- ./$(OBJDIR)thread
	sort mt.out | grep -v 5 > mt.out.sort
	$(PIN_DIFF) mt.out.sort mt.reference
	rm mt.failed mt.out mt.out.sort

kthread.test: $(OBJDIR)kthread kthread.tested kthread.failed
	$(PIN) -- ./$(OBJDIR)kthread > $<.out 2>&1
	$(PIN_DIFF) $<.out kthread.reference
	rm kthread.failed $<.out

callsp_ia32.test: $(OBJDIR)callsp_ia32 callsp_ia32.tested callsp_ia32.failed
	$(PIN) -- ./$(OBJDIR)callsp_ia32
	rm callsp_ia32.failed

callsp_intel64.test: $(OBJDIR)callsp_intel64 callsp_intel64.tested callsp_intel64.failed
	$(PIN) -- ./$(OBJDIR)callsp_intel64
	rm callsp_intel64.failed

mtstatic.test : $(OBJDIR)thread_static mtstatic.tested mtstatic.failed
	$(PIN)  -- ./$(OBJDIR)thread_static >  $<.out 2>&1
	$(PIN_DIFF) $<.out mtstatic.reference
	rm mtstatic.failed $<.out

sigmask-mac.test: $(OBJDIR)sigmask sigmask-mac.tested sigmask-mac.failed
	$(PIN) -- ./$(OBJDIR)sigmask >  $*.out 2>&1
	$(PIN_DIFF) $*.out sigmask-mac.reference
	rm sigmask-mac.failed $*.out

sigmask.test: $(OBJDIR)sigmask sigmask.tested sigmask.failed
	$(PIN) -- ./$(OBJDIR)sigmask >  $<.out 2>&1
	$(PIN_DIFF) $<.out sigmask.reference
	rm sigmask.failed $<.out

sigmaskstatic.test: $(OBJDIR)sigmaskstatic sigmaskstatic.tested sigmaskstatic.failed
	$(PIN) -- ./$(OBJDIR)sigmaskstatic >  $<.out 2>&1
	$(PIN_DIFF) $<.out sigmask.reference
	rm sigmaskstatic.failed $<.out

$(OBJDIR)align_access_app: align_access_app.c 
	${CC} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) 

$(OBJDIR)swizzlealloc: swizzlealloc.c $(SWIZZLETEST_ASM_OBJ)
	${CC} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SWIZZLETEST_ASM_OBJ)

$(OBJDIR)tool_fetch_app: tool_fetch_app.c 
	${CC} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) 

$(OBJDIR)df_test_app: df_test_app.c $(OBJDIR)$(DF_TEST_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)$(DF_TEST_APP_ASM_OBJ) 

$(OBJDIR)reg_operands_test_app: reg_operands_test_app.c $(OBJDIR)$(REG_OPERANDS_TEST_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)$(REG_OPERANDS_TEST_APP_ASM_OBJ) 

$(OBJDIR)seg_override_app1: seg_override_app1.c $(OBJDIR)$(SEG_OVERRIDE_APP1_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(LINK_OPTION) $(OBJDIR)$(SEG_OVERRIDE_APP1_ASM_OBJ) 

$(OBJDIR)analysis_flag_overwrite_app: analysis_flag_overwrite_app.c $(OBJDIR)$(ANALYSIS_FLAG_OVERWRITE_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)$(ANALYSIS_FLAG_OVERWRITE_APP_ASM_OBJ) 

$(OBJDIR)flag_spill_fill_app: flag_spill_fill_app.c $(OBJDIR)$(FLAG_SPILL_FILL_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)$(FLAG_SPILL_FILL_APP_ASM_OBJ) 

$(OBJDIR)flag_ac_app: flag_ac_app.c  $(FLAG_AC_APP_ASM_LIN_SRC)
	$(CC) $(APP_CXXFLAGS) -o $@ flag_ac_app.c  $(FLAG_AC_APP_ASM_LIN_SRC)

$(OBJDIR)flag_ac_win_app: flag_ac_win_app.c $(OBJDIR)$(FLAG_AC_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)$(FLAG_AC_APP_ASM_OBJ) 

$(OBJDIR)jcx_addr16_app: jcx_addr16_app.c  
	$(CC) $(APP_CXXFLAGS) -o $@ jcx_addr16_app.c  

# Assembler file tstcmpxchg8b.c
$(OBJDIR)cmpxchg8b_ms.obj: cmpxchg8b_ms.asm 
	ml /nologo /c /Fo$@ $<

$(OBJDIR)cmpxchg8b_with_explicit_ebx_ms.obj: cmpxchg8b_with_explicit_ebx_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)cmpxchg8b_gnu.o: cmpxchg8b_gnu.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)tstcmpxchg8b.$(OBJEXT): tstcmpxchg8b.c
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)tstcmpxchg8b_with_explicit_ebx.$(OBJEXT): tstcmpxchg8b_with_explicit_ebx.c
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)tstcmpxchg8b$(EXEEXT): $(OBJDIR)tstcmpxchg8b.$(OBJEXT) $(OBJDIR)$(CMPXCHG8_ASM_OBJ)
	$(CC) $(APP_CXXFLAGS) $(OBJDIR)tstcmpxchg8b.$(OBJEXT) $(OBJDIR)$(CMPXCHG8_ASM_OBJ) ${OUTEXE}$@

$(OBJDIR)tstcmpxchg8b_with_explicit_ebx$(EXEEXT): $(OBJDIR)tstcmpxchg8b_with_explicit_ebx.$(OBJEXT) $(OBJDIR)$(CMPXCHG8_WITH_EXPLICIT_EBX_ASM_OBJ)
	$(CC) $(APP_CXXFLAGS)  $(OBJDIR)tstcmpxchg8b_with_explicit_ebx.$(OBJEXT) $(OBJDIR)$(CMPXCHG8_WITH_EXPLICIT_EBX_ASM_OBJ) ${OUTEXE}$@

$(OBJDIR)df_test_app_ms.obj: df_test_app_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)reg_operands_test_app_ms.obj: reg_operands_test_app_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)seg_override_app1_ms.obj: seg_override_app1_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)segmented_ea_verifier_win1_ms.obj: segmented_ea_verifier_win1_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)ea_verifier_addr16_ms.obj: ea_verifier_addr16_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)analysis_flag_overwrite_app_ms.obj: analysis_flag_overwrite_app_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)flag_spill_fill_app_ms.obj: flag_spill_fill_app_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)flag_ac_app_ms.obj: flag_ac_app_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)flag_ac_app_m_intel64.obj: flag_ac_app_m_intel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)segmented_ea_verifier_lin_$(TARGET).o: segmented_ea_verifier_lin_$(TARGET).s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)ea_verifier_lin_addr16_$(TARGET).o: ea_verifier_lin_addr16_$(TARGET).s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

checkinline_nocmov.test: $(OBJDIR)checkinline$(PINTOOL_SUFFIX) checkinline_nocmov.tested checkinline_nocmov.failed $(TESTAPP)
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -nocmov 1 -t $< -o checkinline_nocmov.out -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	rm $<.makefile.copy $(@:.test=.failed) checkinline_nocmov.out

inscount2_fornoinline.test: $(OBJDIR)inscount2_fornoinline$(PINTOOL_SUFFIX) $(TESTAPP)  inscount2_fornoinline.tested inscount2_fornoinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- $(TESTAPP) makefile inscount2_fornoinline.makefile.copy
	$(PIN_DIFF) makefile inscount2_fornoinline.makefile.copy
	rm inscount2_fornoinline.failed inscount2_fornoinline.tested inscount2_fornoinline.makefile.copy	

inline_opt_test.test: $(OBJDIR)inline_opt_test$(PINTOOL_SUFFIX) $(OBJDIR)inline_opt_test_app$(EXEEXT) inline_opt_test.tested inline_opt_test.failed
	$(PIN) -xyzzy -inline_maxlen 1 -log_inline -logfile inline_opt_test.log -t $< -- ./$(OBJDIR)inline_opt_test_app >  $<.out 2>&1
	grep "is too long to inline - is recorded as writing registers" inline_opt_test.log
	rm inline_opt_test.failed $<.out inline_opt_test.log

inline_opt_test_df1.test: $(OBJDIR)inline_opt_test_df1$(PINTOOL_SUFFIX) $(OBJDIR)inline_opt_test_df_app$(EXEEXT) inline_opt_test_df1.tested inline_opt_test_df1.failed
	$(PIN) -xyzzy -inline_maxlen 1 -log_inline -logfile inline_opt_test_df1.log -t $< -- ./$(OBJDIR)inline_opt_test_df_app >  $<.out 2>&1
	grep "is too long to inline - is recorded as writing registers" inline_opt_test_df1.log
	rm inline_opt_test_df1.failed $<.out inline_opt_test_df1.log

inline_opt_test_df2.test: $(OBJDIR)inline_opt_test_df1$(PINTOOL_SUFFIX) $(OBJDIR)inline_opt_test_df_app$(EXEEXT) inline_opt_test_df2.tested inline_opt_test_df2.failed
	$(PIN) -xyzzy -use_sahf 0 -inline_maxlen 1 -log_inline -logfile inline_opt_test_df2.log -t $< -- ./$(OBJDIR)inline_opt_test_df_app >  $<.out 2>&1
	grep "is too long to inline - is recorded as writing registers" inline_opt_test_df2.log
	rm inline_opt_test_df2.failed $<.out inline_opt_test_df2.log

inline_opt_test_df3.test: $(OBJDIR)inline_opt_test_df3$(PINTOOL_SUFFIX) $(OBJDIR)inline_opt_test_df_app$(EXEEXT) inline_opt_test_df3.tested inline_opt_test_df3.failed
	$(PIN) -xyzzy -inline_maxlen 1 -txrep 0 -log_inline -logfile inline_opt_test_df3.log -t $< -- ./$(OBJDIR)inline_opt_test_df_app >  $<.out 2>&1
	grep "is too long to inline - is recorded as writing registers" inline_opt_test_df3.log
	rm inline_opt_test_df3.failed $<.out inline_opt_test_df3.log

reg_operands_test.test: $(OBJDIR)reg_operands_test_tool$(PINTOOL_SUFFIX) $(OBJDIR)reg_operands_test_app reg_operands_test.tested reg_operands_test.failed
	$(PIN)  -t $< -- ./$(OBJDIR)reg_operands_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out reg_operands_test.reference
	rm reg_operands_test.failed $<.out

align_access.test: $(OBJDIR)align_access$(PINTOOL_SUFFIX) $(OBJDIR)align_access_app $(OBJDIR)align_access.tested align_access.failed
	$(PIN)  -t $< -- ./$(OBJDIR)align_access_app >  $<.out 2>&1
	rm align_access.failed align_check.out

df_test1_inline.test: $(OBJDIR)df_test_tool1$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test1_inline.tested df_test1_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test1_inline.reference
	rm df_test1_inline.failed $<.out

df_test1_noinline.test: $(OBJDIR)df_test_tool1$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test1_noinline.tested df_test1_noinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test1_noinline.reference
	rm df_test1_noinline.failed $<.out

df_test1_noinline_bridge.test: %.test : $(OBJDIR)df_test_tool1$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test1_noinline_bridge.reference
	rm df_test1_noinline_bridge.failed $<.out

df_test2_inline.test: $(OBJDIR)df_test_tool2$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test2_inline.tested df_test2_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test2_inline.reference
	rm df_test2_inline.failed $<.out

move_then_test_inline.test: $(OBJDIR)move_then_test$(PINTOOL_SUFFIX)  move_then_test_inline.tested move_then_test_inline.failed
	touch $<.1makefile.copy; rm $<.1makefile.copy
	$(PIN) -xyzzy -region_max_bbls 1 -inline 1 -inline_bridge 1  -t $<  -- $(TESTAPP) makefile $<.1makefile.copy
	$(PIN_CMP) makefile $<.1makefile.copy
	rm move_then_test_inline.failed   $<.1makefile.copy

move_then_test_noinline.test: $(OBJDIR)move_then_test$(PINTOOL_SUFFIX)  move_then_test_noinline.tested move_then_test_noinline.failed
	touch $<.2makefile.copy; rm $<.2makefile.copy
	$(PIN) -xyzzy -region_max_bbls 1 -inline 0  -t $<  -- $(TESTAPP) makefile $<.2makefile.copy
	$(PIN_CMP) makefile $<.2makefile.copy
	rm move_then_test_noinline.failed   $<.2makefile.copy

move_then_test_noinline_bridge.test: $(OBJDIR)move_then_test$(PINTOOL_SUFFIX)  move_then_test_noinline_bridge.tested move_then_test_noinline_bridge.failed
	touch $<.3makefile.copy; rm $<.3makefile.copy
	$(PIN) -xyzzy -region_max_bbls 1 -inline 0 -inline_bridge 0  -t $<  -- $(TESTAPP) makefile $<.3makefile.copy
	$(PIN_CMP) makefile $<.3makefile.copy
	rm move_then_test_noinline_bridge.failed   $<.3makefile.copy

move_then_test_inline_exception.test: $(OBJDIR)move_then_test$(PINTOOL_SUFFIX)  move_then_test_inline_exception.tested move_then_test_inline_exception.failed
	touch $<.4makefile.copy; rm $<.4makefile.copy
	-$(PIN) -xyzzy -region_max_bbls 1 -inline 1 -inline_bridge 1 -generate_then_code_exception 1 -mesgon warning -logfile move_then_test_inline_exception.log -t $<  -- $(TESTAPP) makefile $<4.makefile.copy >  $<.out 2>&1
	echo "move_then_test_inline_exception.test pin execution should fail.  Ignore the error (above, but not one below!)."
	grep "Exception occurred in the code-cache but NOT in any code section" move_then_test_inline_exception.log
	rm move_then_test_inline_exception.failed move_then_test_inline_exception.log  

move_then_test_noinline_exception.test: $(OBJDIR)move_then_test$(PINTOOL_SUFFIX)  move_then_test_noinline_exception.tested move_then_test_noinline_exception.failed
	touch $<.5makefile.copy; rm $<.5makefile.copy
	-$(PIN) -xyzzy -region_max_bbls 1 -inline 0 -inline_bridge 1 -generate_then_code_exception 1 -mesgon warning -logfile move_then_test_noinline_exception.log -t $<  -- $(TESTAPP) makefile $<.5makefile.copy >  $<.out 2>&1
	echo "move_then_test_noinline_exception.test pin execution should fail.  Ignore the error (above, but not one below!)."
	grep "Exception occurred in the code-cache but NOT in any code section" move_then_test_noinline_exception.log
	rm move_then_test_noinline_exception.failed  move_then_test_noinline_exception.log 

move_then_test_noinline_bridge_exception.test: $(OBJDIR)move_then_test$(PINTOOL_SUFFIX)  move_then_test_noinline_bridge_exception.tested move_then_test_noinline_bridge_exception.failed
	touch $<.6makefile.copy; rm $<.6makefile.copy
	-$(PIN) -xyzzy -region_max_bbls 1 -inline 0 -inline_bridge 0 -generate_then_code_exception 1 -mesgon warning -logfile move_then_test_noinline_bridge_exception.log -t $<  -- $(TESTAPP) makefile $<.6makefile.copy >  $<.out 2>&1
	echo "move_then_test_noinline_bridge_exception.test pin execution should fail.  Ignore the error (above, but not one below!)."
	grep "Exception occurred in the code-cache but NOT in any code section" move_then_test_noinline_bridge_exception.log
	rm move_then_test_noinline_bridge_exception.failed move_then_test_noinline_bridge_exception.log  

set_fp_context_xmm_regs.test : $(OBJDIR)set_fp_context_xmm_regs$(PINTOOL_SUFFIX) $(OBJDIR)$(SET_FP_CONTEXT_XMM_REGS_APP)  set_fp_context_xmm_regs.tested set_fp_context_xmm_regs.failed 
	$(PIN) -t $< -- ./$(OBJDIR)$(SET_FP_CONTEXT_XMM_REGS_APP) >  $<.out 2>&1 
ifeq ($(TARGET),ia32)
	$(PIN_DIFF) $<.out set_fp_context_xmm_regs_ia32.reference 
else
	$(PIN_DIFF) $<.out set_fp_context_xmm_regs_ia32e.reference 
endif
	rm set_fp_context_xmm_regs.failed $<.out 

df_test2_noinline.test: $(OBJDIR)df_test_tool2$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test2_noinline.tested df_test2_noinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test2_noinline.reference
	rm df_test2_noinline.failed $<.out

df_test2_noinline_bridge.test: %.test : $(OBJDIR)df_test_tool2$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test2_noinline_bridge.reference
	rm df_test2_noinline_bridge.failed $<.out

df_test3_inline.test: $(OBJDIR)df_test_tool3$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test3_inline.tested df_test3_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test3_inline.reference
	rm df_test3_inline.failed $<.out

df_test3_noinline.test: %.test : $(OBJDIR)df_test_tool3$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test3_noinline.reference
	rm df_test3_noinline.failed $<.out

df_test3_noinline_bridge.test: %.test : $(OBJDIR)df_test_tool3$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test3_noinline_bridge.reference
	rm df_test3_noinline_bridge.failed $<.out

df_test4_inline.test: $(OBJDIR)df_test_tool4$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test4_inline.tested df_test4_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)df_test_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test4_inline.reference
	rm df_test4_inline.failed $<.out

df_test4_noinline.test: $(OBJDIR)df_test_tool4$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test4_noinline.tested df_test4_noinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)df_test_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test4_noinline.reference
	rm df_test4_noinline.failed $<.out

df_test4_noinline_bridge.test: %.test : $(OBJDIR)df_test_tool4$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)df_test_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test4_noinline_bridge.reference
	rm df_test4_noinline_bridge.failed $<.out

df_test5_inline.test: $(OBJDIR)df_test_tool5$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app df_test5_inline.tested df_test5_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)df_test_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test5_inline.reference
	rm df_test5_inline.failed $<.out

df_test5_noinline.test: %.test : $(OBJDIR)df_test_tool5$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)df_test_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test5_noinline.reference
	rm df_test5_noinline.failed $<.out

df_test5_noinline_bridge.test: %.test : $(OBJDIR)df_test_tool5$(PINTOOL_SUFFIX) $(OBJDIR)df_test_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)df_test_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out df_test5_noinline_bridge.reference
	rm df_test5_noinline_bridge.failed $<.out

flag_spill_fill_test1_inline.test: $(OBJDIR)flag_spill_fill_tool1$(PINTOOL_SUFFIX) $(OBJDIR)flag_spill_fill_app flag_spill_fill_test1_inline.tested flag_spill_fill_test1_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)flag_spill_fill_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out flag_spill_fill_test1_inline.reference
	rm flag_spill_fill_test1_inline.failed $<.out

flag_spill_fill_test1_noinline.test: $(OBJDIR)flag_spill_fill_tool1$(PINTOOL_SUFFIX) $(OBJDIR)flag_spill_fill_app flag_spill_fill_test1_noinline.tested flag_spill_fill_test1_noinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)flag_spill_fill_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out flag_spill_fill_test1_noinline.reference
	rm flag_spill_fill_test1_noinline.failed $<.out

flag_spill_fill_test1_noinline_bridge.test: $(OBJDIR)flag_spill_fill_tool1$(PINTOOL_SUFFIX) $(OBJDIR)flag_spill_fill_app flag_spill_fill_test1_noinline_bridge.tested flag_spill_fill_test1_noinline_bridge.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)flag_spill_fill_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out flag_spill_fill_test1_noinline_bridge.reference
	rm flag_spill_fill_test1_noinline_bridge.failed $<.out

analysis_flag_overwrite_test1_inline.test: %.test: $(OBJDIR)analysis_flag_overwrite_tool1$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test1_inline.reference
	rm analysis_flag_overwrite_test1_inline.failed $<.out

analysis_flag_overwrite_test1_noinline.test: %.test: $(OBJDIR)analysis_flag_overwrite_tool1$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test1_noinline.reference
	rm analysis_flag_overwrite_test1_noinline.failed $<.out

analysis_flag_overwrite_test1_noinline_bridge.test: %.test: $(OBJDIR)analysis_flag_overwrite_tool1$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test1_noinline_bridge.reference
	rm analysis_flag_overwrite_test1_noinline_bridge.failed $<.out

analysis_flag_overwrite_test2_inline.test: %.test: $(OBJDIR)analysis_flag_overwrite_tool2$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test2_inline.reference
	rm analysis_flag_overwrite_test2_inline.failed $<.out

analysis_flag_overwrite_test2_noinline.test: %.test: $(OBJDIR)analysis_flag_overwrite_tool2$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test2_noinline.reference
	rm analysis_flag_overwrite_test2_noinline.failed $<.out

analysis_flag_overwrite_test2_noinline_bridge.test:  %.test:$(OBJDIR)analysis_flag_overwrite_tool2$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test2_noinline_bridge.reference
	rm analysis_flag_overwrite_test2_noinline_bridge.failed $<.out

analysis_flag_overwrite_test3_inline.test: %.test: $(OBJDIR)analysis_flag_overwrite_tool3$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test3_inline.reference
	rm analysis_flag_overwrite_test3_inline.failed $<.out

analysis_flag_overwrite_test3_noinline.test:  %.test: $(OBJDIR)analysis_flag_overwrite_tool3$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test3_noinline.reference
	rm analysis_flag_overwrite_test3_noinline.failed $<.out

analysis_flag_overwrite_test3_noinline_bridge.test:  %.test: $(OBJDIR)analysis_flag_overwrite_tool3$(PINTOOL_SUFFIX) $(OBJDIR)analysis_flag_overwrite_app %.tested %.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)analysis_flag_overwrite_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out analysis_flag_overwrite_test3_noinline_bridge.reference
	rm analysis_flag_overwrite_test3_noinline_bridge.failed $<.out

flag_ac_noinline_bridge.test: $(OBJDIR)flag_ac_tool$(PINTOOL_SUFFIX) $(OBJDIR)flag_ac_app flag_ac_noinline_bridge.tested flag_ac_noinline_bridge.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)flag_ac_app  >  $@.out 2>&1
	$(PIN_DIFF) $@.out flag_ac_noinline_bridge.reference
	rm flag_ac_noinline_bridge.failed $@.out

flag_ac_noinline.test: $(OBJDIR)flag_ac_tool$(PINTOOL_SUFFIX) $(OBJDIR)flag_ac_app flag_ac_noinline.tested flag_ac_noinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)flag_ac_app  >  $@.out 2>&1
	$(PIN_DIFF) $@.out flag_ac_noinline.reference
	rm flag_ac_noinline.failed $@.out

flag_ac_inline.test: $(OBJDIR)flag_ac_tool$(PINTOOL_SUFFIX) $(OBJDIR)flag_ac_app flag_ac_inline.tested flag_ac_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)flag_ac_app  >  $@.out 2>&1
	$(PIN_DIFF) $@.out flag_ac_inline.reference
	rm flag_ac_inline.failed $@.out

flag_ac_win_noinline_bridge.test: $(OBJDIR)flag_ac_win_tool$(PINTOOL_SUFFIX) $(OBJDIR)flag_ac_win_app flag_ac_win_noinline_bridge.tested flag_ac_win_noinline_bridge.failed
	$(PIN) -xyzzy -inline 0 -inline_bridge 0 -t $< -- ./$(OBJDIR)flag_ac_win_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out flag_ac_win_noinline_bridge.reference
	rm flag_ac_win_noinline_bridge.failed $<.out

flag_ac_win_noinline.test: $(OBJDIR)flag_ac_win_tool$(PINTOOL_SUFFIX) $(OBJDIR)flag_ac_win_app flag_ac_win_noinline.tested flag_ac_win_noinline.failed
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)flag_ac_win_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out flag_ac_win_noinline.reference
	rm flag_ac_win_noinline.failed $<.out

flag_ac_win_inline.test: $(OBJDIR)flag_ac_win_tool$(PINTOOL_SUFFIX) $(OBJDIR)flag_ac_win_app flag_ac_win_inline.tested flag_ac_win_inline.failed
	$(PIN) -xyzzy -inline 1 -t $< -- ./$(OBJDIR)flag_ac_win_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out flag_ac_win_inline.reference
	rm flag_ac_win_inline.failed $<.out

swizzle2.test: $(OBJDIR)swizzle2$(PINTOOL_SUFFIX) $(OBJDIR)swizzlealloc swizzle2.tested swizzle2.failed
	$(PIN) -t $< -- ./$(OBJDIR)swizzlealloc >  $<.out 2>&1
	$(PIN_DIFF) $<.out swizzle2.reference
	rm swizzle2.failed $<.out

swizzle3.test: $(OBJDIR)swizzle3$(PINTOOL_SUFFIX) $(OBJDIR)swizzlealloc swizzle3.tested swizzle3.failed
	$(PIN) -t $< -- ./$(OBJDIR)swizzlealloc >  $<.out 2>&1
	$(PIN_DIFF) $<.out swizzle2.reference
	rm swizzle3.failed $<.out

swizzle5.test: $(OBJDIR)swizzle5$(PINTOOL_SUFFIX) $(OBJDIR)swizzlealloc swizzle5.tested swizzle5.failed
	$(PIN) -t $< -- ./$(OBJDIR)swizzlealloc >  $<.out 2>&1
	$(PIN_DIFF) $<.out swizzle5.reference
	rm swizzle5.failed $<.out

obj-ia32/smallpush: smallpush.s
	$(AS) $(AS_FLAGS) -o $@.o smallpush.s
	$(LD) $(ASLD_FLAGS) -o $@ $@.o

obj-intel64/smallpush: smallpush64.s
	$(AS) $(AS_FLAGS) -o $@.o smallpush64.s
	$(LD) $(ASLD_FLAGS) -o $@ $@.o

smallpush.test: $(OBJDIR)smallpush smallpush.tested smallpush.failed
	# This test should exit with status zero. Any other exit code tells you which
	# test failed.
	$(PIN) -- ./$(OBJDIR)smallpush
	rm smallpush.failed

popea_verifier.test: $(OBJDIR)popea_verifier$(PINTOOL_SUFFIX) $(OBJDIR)smallpush$(EXEEXT) popea_verifier.tested popea_verifier.failed
	$(PIN) -t $< -- ./$(OBJDIR)smallpush$(EXEEXT) > $<.out 2>&1
	rm popea_verifier.failed popea_verifier.out $<.out

rewritememop.test: $(OBJDIR)rewritememop$(PINTOOL_SUFFIX) $(OBJDIR)tstcmpxchg8b$(EXEEXT) rewritememop.tested rewritememop.failed
	$(PIN) -t $< -- ./$(OBJDIR)tstcmpxchg8b$(EXEEXT) > $<.out 2>&1
	rm rewritememop.failed rewritememtrace.out $<.out

# Memory address rewriting tests which use the new memory rewriting interface
# 
$(OBJDIR)stringtest$(EXEEXT): stringtest.c
	$(CC) $(APP_CXXFLAGS) -o $@ stringtest.c 

# Tests rewriting both memory operands
rwm1_str.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)stringtest$(EXEEXT) rwm1_str.tested rwm1_str.failed
	$(PIN) -t $< -o rwm_str.out -- ./$(OBJDIR)stringtest$(EXEEXT) > $(OBJDIR)str.out 2>&1
	rm rwm1_str.failed rwm_str.out $(OBJDIR)str.out

rwm_farret.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)far$(EXEEXT) rwm_farret.tested rwm_farret.failed
	$(PIN) -t $< -o rwm_farret_tool.out -- ./$(OBJDIR)far$(EXEEXT) > $(OBJDIR)farret.out 2>&1
	rm rwm_farret.failed rwm_farret_tool.out $(OBJDIR)farret.out

rwm_pushapopa.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)pusha_popa$(EXEEXT) rwm_pushapopa.tested rwm_pushapopa.failed
	$(PIN) -t $< -o rwm_pushapopa_tool.out -- ./$(OBJDIR)pusha_popa$(EXEEXT) > $(OBJDIR)pushapopa.out 2>&1
	rm rwm_pushapopa.failed rwm_pushapopa_tool.out $(OBJDIR)pushapopa.out

rwm_pushtest.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)pushtest$(EXEEXT) rwm_pushtest.tested rwm_pushtest.failed
	$(PIN) -t $< -o rwm_pushtest_tool.out -- ./$(OBJDIR)pushtest$(EXEEXT) > $(OBJDIR)rwm_pushtest.out 2>&1
	rm rwm_pushtest.failed $(OBJDIR)rwm_pushtest.out rwm_pushtest_tool.out

rwm_smallpush.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)smallpush$(EXEEXT) rwm_smallpush.tested rwm_smallpush.failed
	$(PIN) -t $< -o rwm_smallpush.out -- ./$(OBJDIR)smallpush$(EXEEXT) > $(OBJDIR)smallpush.out 2>&1
	rm rwm_smallpush.failed rwm_smallpush.out $(OBJDIR)smallpush.out

# Tests rewriting only the first memory operand
rwm1_str1.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)stringtest$(EXEEXT) rwm1_str1.tested rwm1_str1.failed
	$(PIN) -t $< -2 -o rwm1_str1.out -- ./$(OBJDIR)stringtest$(EXEEXT) > $(OBJDIR)str1.out 2>&1
	rm rwm1_str1.failed rwm1_str1.out $(OBJDIR)str1.out

rwm_farret1.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)far$(EXEEXT) rwm_farret1.tested rwm_farret1.failed
	$(PIN) -t $< -2 -o rwm_farret1.out -- ./$(OBJDIR)far$(EXEEXT) > $(OBJDIR)farret1.out 2>&1
	rm rwm_farret1.failed rwm_farret1.out $(OBJDIR)farret1.out

rwm_pushapopa1.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)pusha_popa$(EXEEXT) rwm_pushapopa1.tested rwm_pushapopa1.failed
	$(PIN) -t $< -2 -o rwm_pushapopa1.out -- ./$(OBJDIR)pusha_popa$(EXEEXT) > $(OBJDIR)pushapopa1.out 2>&1
	rm rwm_pushapopa1.failed rwm_pushapopa1.out $(OBJDIR)pushapopa1.out

rwm_pushtest1.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)pushtest$(EXEEXT) rwm_pushtest1.tested rwm_pushtest1.failed
	$(PIN) -t $< -2 -o rwm_pushtest1.out -- ./$(OBJDIR)pushtest$(EXEEXT) > $(OBJDIR)pushtest1.out 2>&1
	rm rwm_pushtest1.failed rwm_pushtest1.out $(OBJDIR)pushtest1.out

rwm_smallpush1.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)smallpush$(EXEEXT) rwm_smallpush1.tested rwm_smallpush1.failed
	$(PIN) -t $< -2 -o rwm_smallpush1.out -- ./$(OBJDIR)smallpush$(EXEEXT) > $(OBJDIR)smallpush1.out 2>&1
	rm rwm_smallpush1.failed rwm_smallpush1.out $(OBJDIR)smallpush1.out

rwm_bitops.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)btc_app$(EXEEXT) rwm_bitops.tested rwm_bitops.failed
	$(PIN) -t $< -2 -o rwm_bitops.out -- ./$(OBJDIR)btc_app$(EXEEXT) > $(OBJDIR)btc_app.out 2>&1
	rm rwm_bitops.failed rwm_bitops.out $(OBJDIR)btc_app.out

# Tests rewriting only the second memory operand
rwm1_str2.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)stringtest$(EXEEXT) rwm1_str2.tested rwm1_str2.failed
	$(PIN) -t $< -1 -o rwm1_str2.out -- ./$(OBJDIR)stringtest$(EXEEXT) > $(OBJDIR)rwm1_str2.out 2>&1
	rm rwm1_str2.failed rwm1_str2.out $(OBJDIR)rwm1_str2.out

rwm_farret2.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)far$(EXEEXT) rwm_farret2.tested rwm_farret2.failed
	$(PIN) -t $< -1 -o rwm_farret2.out -- ./$(OBJDIR)far$(EXEEXT) > $(OBJDIR)rwm_farret2.out 2>&1
	rm rwm_farret2.failed rwm_farret2.out $(OBJDIR)rwm_farret2.out

rwm_pushapopa2.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)pusha_popa$(EXEEXT) rwm_pushapopa2.tested rwm_pushapopa2.failed
	$(PIN) -t $< -1 -o rwm_pushapopa2.out -- ./$(OBJDIR)pusha_popa$(EXEEXT) > $(OBJDIR)rwm_pushapopa2.out 2>&1
	rm rwm_pushapopa2.failed rwm_pushapopa2.out $(OBJDIR)rwm_pushapopa2.out

rwm_pushtest2.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)pushtest$(EXEEXT) rwm_pushtest2.tested rwm_pushtest2.failed
	$(PIN) -t $< -1 -o rwm_pushtest2.out -- ./$(OBJDIR)pushtest$(EXEEXT) > $(OBJDIR)rwm_pushtest2.out 2>&1
	rm rwm_pushtest2.failed rwm_pushtest2.out $(OBJDIR)rwm_pushtest2.out

rwm_smallpush2.test: $(OBJDIR)rewritememop1$(PINTOOL_SUFFIX) $(OBJDIR)smallpush$(EXEEXT) rwm_smallpush2.tested rwm_smallpush2.failed
	$(PIN) -t $< -1 -o rwm_smallpush2.out -- ./$(OBJDIR)smallpush$(EXEEXT) > $(OBJDIR)rwm_smallpush2.out 2>&1
	rm rwm_smallpush2.failed rwm_smallpush2.out $(OBJDIR)rwm_smallpush2.out

tstcmpxchg8b_with_explicit_ebx.test:  $(OBJDIR)tstcmpxchg8b_with_explicit_ebx$(EXEEXT) tstcmpxchg8b_with_explicit_ebx.tested tstcmpxchg8b_with_explicit_ebx.failed
	$(PIN)  -- ./$(OBJDIR)tstcmpxchg8b_with_explicit_ebx$(EXEEXT) > $<.out 2>&1
	$(PIN_DIFF) $<.out tstcmpxchg8b_with_explicit_ebx.reference
	rm tstcmpxchg8b_with_explicit_ebx.failed  $<.out

safecopy.test : $(OBJDIR)safecopy$(PINTOOL_SUFFIX) safecopy.tested safecopy.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -o safecopy.out -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	rm safecopy.failed  safecopy.out $<.makefile.copy

create_rtn.test: %.test : $(OBJDIR)%$(PINTOOL_SUFFIX) $(OBJDIR)create_rtn_app$(EXEEXT) %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)create_rtn_app$(EXEEXT) > $*_jit.out 2>&1
	grep "replaced successfully" $*_jit.out
	grep "call was inserted successfully" $*_jit.out
	rm $*_jit.out $*.failed

create_rtn_probed.test: %.test : $(OBJDIR)create_rtn$(PINTOOL_SUFFIX) $(OBJDIR)create_rtn_app$(EXEEXT) %.tested %.failed
	$(PIN) -t $< -probe_mode 1 -- $(OBJDIR)create_rtn_app$(EXEEXT) > $*_probe.out 2>&1
	grep "replaced successfully" $*_probe.out
	grep "call was inserted successfully" $*_probe.out
	rm $*_probe.out $*.failed

claim_regs.test: %.test : $(OBJDIR)claim_regs$(PINTOOL_SUFFIX) %.tested %.failed
	touch $*.tested $*.failed
	$(PIN) -t $< -- $(TESTAPP) > $*.out 2>&1
	grep "Passed" $*.out
	rm $*.out $*.failed

checkerrorfile.test: %.test : $(OBJDIR)checkerrorfile$(PINTOOL_SUFFIX) %.tested %.failed
	touch $*.tested $*.failed
	- $(PIN) -t $< -- $(TESTAPP) > $*.out 2>&1
	echo "checkerrorfile pin execution should fail.  Ignore the error (above, but not one below!)."
	grep "checkerrorfile.cpp:[14]7" $*.out
	rm $*.out $*.failed

$(OBJDIR)kthread: kthread.c $(THREAD_LIB)
	$(CC) $(APP_CXXFLAGS)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

$(OBJDIR)thread: thread.c $(THREAD_LIB)
	$(CC) $(APP_CXXFLAGS)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

$(OBJDIR)thread_static: thread.c $(THREAD_LIB)
	$(CC) $(APP_CXXFLAGS) $(STATIC) $< ${OUTEXE}$@ $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

$(OBJDIR)thread_longshort: thread_longshort.c  $(THREAD_LIB)
	$(CC) $(APP_CXXFLAGS)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

$(OBJDIR)thread_segmented_ea: thread_segmented_ea.c  $(THREAD_LIB) $(SEGMENTED_EA_VERIFIER_ASM_OBJ)
	$(CC) $(APP_CXXFLAGS)  ${OUTEXE}$@ $< $(LINK_OPTION) $(THREAD_LIB) $(APP_PTHREAD) $(SEGMENTED_EA_VERIFIER_ASM_OBJ)

$(OBJDIR)thread_ea_addr16: thread_ea_addr16.c  $(THREAD_LIB) $(EA_VERIFIER_ADDR16_ASM_OBJ)
	$(CC) $(APP_CXXFLAGS)  ${OUTEXE}$@ $< $(LINK_OPTION) $(THREAD_LIB) $(APP_PTHREAD) $(EA_VERIFIER_ADDR16_ASM_OBJ)

$(OBJDIR)callsp_ia32: callsp_ia32.s
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ callsp_ia32.s

$(OBJDIR)callsp_intel64: callsp_intel64.s
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ callsp_intel64.s

$(OBJDIR)sigmaskstatic: sigmask.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ sigmask.c -static

$(OBJDIR)sigmask: sigmask.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ sigmask.c 

$(OBJDIR)create_rtn_app : create_rtn_app.cpp
	$(CXX) $(APP_CXXFLAGS) $< -o $@ -Wl,--strip-all

$(OBJDIR)create_rtn_app.exe : create_rtn_app.cpp
	$(CXX) $(APP_CXXFLAGS) $< ${OUTEXE}$@ 

$(OBJDIR)win_exception.exe : win_exception.c
	$(CC) $(APP_CXXFLAGS) $< ${OUTEXE}$@ 


findthreadwithappstack.test : %.test :$(OBJDIR)malmalloc$(PINTOOL_SUFFIX) $(OBJDIR)thread_longshort %.tested %.failed
	$(PIN) -xyzzy -switchstack 0 -t $< -- ./$(OBJDIR)thread_longshort >  $<.out 2>&1
	$(PIN_DIFF) $<.out findthreadwithappstack.reference
	rm findthreadwithappstack.failed $<.out

$(OBJDIR)dummy.so : dummy.c
	$(CC) $(APP_CXXFLAGS) -shared $< ${OUTOPT}$@

stackunswitch.test: $(OBJDIR)stackunswitch$(PINTOOL_SUFFIX) $(OBJDIR)thread stackunswitch.tested stackunswitch.failed
	$(PIN)  -t $< -- ./$(OBJDIR)thread >  $<.out 2>&1
	$(PIN_DIFF) $<.out stackunswitch.reference
	rm stackunswitch.failed $<.out


# setting rpath did not work with cc on mmdcs082
soload.test: $(OBJDIR)soload${PINTOOL_SUFFIX} $(OBJDIR)$(DLTEST) soload.tested soload.failed
	export LD_LIBRARY_PATH=`pwd`\/$(OBJDIR); $(PIN) -t $< -- ./$(OBJDIR)$(DLTEST)
	$(PIN_DIFF) soload.out soload.reference
	rm soload.failed soload.out

soloadrange.test: $(OBJDIR)soloadrange${PINTOOL_SUFFIX} $(OBJDIR)$(DLTEST2) soloadrange.tested soloadrange.failed
	touch soloadrange.reference
	export LD_LIBRARY_PATH=`pwd`\/$(OBJDIR); $(PIN) -t $< -- ./$(OBJDIR)dltest2 > /dev/null 2>&1
	$(PIN_CMP) soloadrange.out soloadrange.reference
	rm soloadrange.failed soloadrange.out soloadrange.reference

# Test that flushing an MT app happens in a timely manner
$(OBJDIR)mtflushapp_unix: mtflushapp_unix.cpp
	$(CXX) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ mtflushapp_unix.cpp $(APP_PTHREAD)

$(OBJDIR)mtflushapp_win.exe: mtflushapp_win.cpp
	$(CXX) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ mtflushapp_win.cpp

mtflush.test: $(OBJDIR)mtflush$(PINTOOL_SUFFIX) $(OBJDIR)$(MTFLUSHAPP) mtflush.tested mtflush.failed
	$(PIN) -t $(OBJDIR)mtflush${PINTOOL_SUFFIX} -- ./$(OBJDIR)$(MTFLUSHAPP)
	$(PIN_DIFF) $(@:.test=.out) $(@:.test=.reference)
	rm mtflush.failed

# Test the automatic resizing of stripes
resize.test: resize.tested resize.failed
	$(PIN) -xyzzy -max_rtn 1024 -recycle_rtn 0 -- $(TESTAPP) makefile resize.makefile.copy
	$(PIN_CMP) makefile resize.makefile.copy
	rm resize.failed resize.makefile.copy

# Test the small secondary stack
nosahflahf.test:  %.test : %.tested %.failed
	$(PIN) -xyzzy -use_sahf 0 -t -- $(TESTAPP) makefile nosahflahf.makefile.copy 
	$(PIN_DIFF) makefile nosahflahf.makefile.copy
	rm nosahflahf.failed nosahflahf.makefile.copy 

# Test reserve memory
$(OBJDIR)reserve_memory: reserve_memory.c
	$(CC) $(APP_CXXFLAGS) -o $@ $< -static

reservemem.test: reservemem.tested reservemem.failed
	$(PIN) -xyzzy -reserve_memory large.address -- $(TESTAPP) makefile reservemem.makefile.copy
	$(PIN_CMP) makefile reservemem.makefile.copy
	rm reservemem.failed reservemem.makefile.copy

reservemem64.test: $(OBJDIR)reserve_memory reservemem64.tested reservemem64.failed reservemem.tested reservemem.failed
	$(PIN) -xyzzy -reserve_memory large.address -- $(TESTAPP) makefile reservemem.makefile.copy
	$(PIN_CMP) makefile reservemem.makefile.copy
	rm reservemem.failed reservemem.makefile.copy
	$(OBJDIR)reserve_memory bigrange.address 0 > reservemem64.native.out
	$(PIN) -xyzzy -reserve_memory bigrange.address -- $(OBJDIR)reserve_memory bigrange.address 1 > reservemem64.pin.out
	cmp reservemem64.native.out reservemem64.pin.out
	rm reservemem64.failed reservemem64.native.out reservemem64.pin.out

partialinline2.test : $(OBJDIR)partialinline$(PINTOOL_SUFFIX) partialinline2.tested partialinline2.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline 0 -t $< -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	rm partialinline2.failed $<.makefile.copy

syscall.test : $(OBJDIR)syscall$(PINTOOL_SUFFIX) syscall.tested syscall.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	rm syscall.failed $<.makefile.copy

inlinecall.test : $(OBJDIR)inlinecall$(PINTOOL_SUFFIX) inlinecall.tested inlinecall.failed $(OBJDIR)$(ONEPROG)
	$(PIN) -t $< -- ./$(OBJDIR)$(ONEPROG) >  $<.out 2>&1
	$(PIN_DIFF) $<.out inlinecall.reference
	rm inlinecall.failed $<.out


strace_ipf.test : $(OBJDIR)strace_ipf$(PINTOOL_SUFFIX) strace_ipf.tested strace_ipf.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -q "Success" strace.out
	grep -q -v "Failure" strace.out
	grep -q "eof" strace.out
	rm strace_ipf.failed strace.out $<.out

strace_ia32.test : $(OBJDIR)strace_ia32$(PINTOOL_SUFFIX) strace_ia32.tested strace_ia32.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -q "Success" strace.out
	grep -q -v "Failure" strace.out
	grep -q "eof" strace.out
	rm strace_ia32.failed strace.out $<.out

dlclose.test: %.test: $(OBJDIR)dlclose$(PINTOOL_SUFFIX) $(OBJDIR)dlclose_app %.tested %.failed
	export LD_LIBRARY_PATH=`pwd`\/$(OBJDIR); $(PIN)  -t $< -- $(OBJDIR)dlclose_app >  $*.out 2>&1
	test `grep -e "Unloaded" $*.out | wc -l` -eq "2"
	rm $*.failed $*.out

stracewin_ia32$(PINTOOL_SUFFIX).test : $(OBJDIR)stracewin_ia32$(PINTOOL_SUFFIX) stracewin_ia32.tested stracewin_ia32.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -q "Success" stracewin.out
	grep -q -v "Failure" stracewin.out
	grep -q "eof" stracewin.out
	rm stracewin_ia32.failed stracewin.out $<.out

context.test : $(OBJDIR)context$(PINTOOL_SUFFIX) context.tested context.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -o $<.out -- $(TESTAPP) makefile $<.makefile.copy
	grep -q -v "Failure" $<.out
	grep -q "Success" $<.out
	rm context.failed $<.out $<.makefile.copy

jitmalloctrace.test : $(OBJDIR)jitmalloctrace$(PINTOOL_SUFFIX) jitmalloctrace.tested jitmalloctrace.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	$(PIN_CMP) makefile $<.makefile.copy
	grep -q "malloc" jitmalloctrace.outfile
	rm jitmalloctrace.failed $<.out $<.makefile.copy jitmalloctrace.outfile

replace_free.test : $(OBJDIR)replace_free$(PINTOOL_SUFFIX) replace_free.tested replace_free.failed $(OBJDIR)$(LITTLE_MALLOC)
	$(PIN) -t $< -- ./$(OBJDIR)$(LITTLE_MALLOC) > $<.out 2>&1
	grep -q "free" $<.out
	rm replace_free.failed $<.out

rtnreplace_free.test : $(OBJDIR)rtnreplace_free$(PINTOOL_SUFFIX) rtnreplace_free.tested rtnreplace_free.failed $(OBJDIR)$(LITTLE_MALLOC)
	$(PIN) -t $< -- ./$(OBJDIR)$(LITTLE_MALLOC) > $<.out 2>&1
	grep -q "free" $<.out
	rm rtnreplace_free.failed $<.out

fp_replace.test : $(OBJDIR)fp_replace$(PINTOOL_SUFFIX) fp_replace.tested fp_replace.failed $(OBJDIR)fp_replace_app
	$(PIN) -t $< -- ./$(OBJDIR)fp_replace_app > $<.out 2>&1
	grep -q "1.00335" $<.out
	rm fp_replace.failed $<.out

fp_insert.test : $(OBJDIR)fp_insert$(PINTOOL_SUFFIX) fp_insert.tested fp_insert.failed $(OBJDIR)fp_replace_app
	$(PIN) -t $< -- ./$(OBJDIR)fp_replace_app > $<.out 2>&1
	grep -q "1.00335" $<.out
	rm fp_insert.failed $<.out

datasym.test : $(OBJDIR)datasym$(PINTOOL_SUFFIX) $(OBJDIR)datasym_app.exe datasym.tested datasym.failed
	rm -f symbolnames.outfile
	-$(PIN) -t $< -- ./$(OBJDIR)datasym_app.exe > $<.out
	grep -c -i "exported_data1" symbolnames.outfile | grep "0"
#	rm symbolnames.outfile $<.out
#	-$(PIN) -t $< -- ./$(OBJDIR)datasym_appdbg.exe > $<.out
#	grep -c -i "exported_data1" symbolnames.outfile | grep "0"
	rm datasym.failed symbolnames.outfile $<.out


short_name.test : $(OBJDIR)short_name$(PINTOOL_SUFFIX) short_name.tested short_name.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep fopen short_name.outfile
	$(PIN) -t $< -use_dynsym -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep fopen: short_name.outfile
	$(PIN) -t $< -use_dynsym -short_name -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep fopen: short_name.outfile
	$(PIN) -t $< -short_name -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep fopen short_name.outfile
	rm short_name.failed $<.out $<.makefile.copy short_name.outfile

replace_malloc_inst.test : $(OBJDIR)replace_malloc_inst$(PINTOOL_SUFFIX) replace_malloc_inst.tested replace_malloc_inst.failed $(OBJDIR)$(LITTLE_MALLOC)
	$(PIN) -t $< -- ./$(OBJDIR)$(LITTLE_MALLOC) >  $<.out 2>&1
	grep -q "Test passed" $<.out
	rm replace_malloc_inst.failed $<.out

thread_callback.test : $(OBJDIR)thread_callback$(PINTOOL_SUFFIX) $(OBJDIR)$(THREADTEST) thread_callback.tested thread_callback.failed
	-$(PIN) -t $< -- ./$(OBJDIR)$(THREADTEST) >  $<.out 2>&1
	echo "thread_callback should fail.  Ignore the error."
	rm thread_callback.failed $<.out

executeat_callback.test : $(OBJDIR)executeat_callback$(PINTOOL_SUFFIX) $(OBJDIR)$(THREADTEST) executeat_callback.tested executeat_callback.failed
	-$(PIN) -t $< -- ./$(OBJDIR)$(THREADTEST) >  $<.out 2>&1
	echo "executeat_callback should fail.  Ignore the error."
	grep -e "PIN_ExecuteAt() cannot be called from a callback." $<.out
	-$(PIN) -error_file $<.xml -t $< -- ./$(OBJDIR)$(THREADTEST) >  $<.out 2>&1
	grep -e "PIN_ExecuteAt() cannot be called from a callback." $<.xml
	rm executeat_callback.failed $<.out $<.xml

failed_attach_with_error_file.test : failed_attach_with_error_file.tested failed_attach_with_error_file.failed
	-$(PIN) -error_file $(OBJDIR)failed_attach_with_error_file.xml -pid 1 >  $(OBJDIR)failed_attach_with_error_file.out 2>&1
	echo "failed_attach_with_error_fileshould fail.  Ignore the error."
	grep -e "Attach to pid" $(OBJDIR)failed_attach_with_error_file.xml
	rm failed_attach_with_error_file.failed $(OBJDIR)failed_attach_with_error_file.out $(OBJDIR)failed_attach_with_error_file.xml 

executeat_lock.test : $(OBJDIR)executeat_lock$(PINTOOL_SUFFIX) $(OBJDIR)$(THREADTEST) executeat_lock.tested executeat_lock.failed
	-$(PIN) -t $< -- ./$(OBJDIR)$(THREADTEST) >  $<.out 2>&1
	echo "executeat_lock should fail.  Ignore the error."
	grep -e "Pin Client Lock" $<.out
	rm executeat_lock.failed $<.out

str2int.test : $(OBJDIR)str2int$(PINTOOL_SUFFIX) $(OBJDIR)$(THREADTEST) str2int.tested str2int.failed
	echo "str2int should fail.  Ignore the error."
	-$(PIN) -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -e "Error in string conversion" $<.out
	-$(PIN) -error_file $<.xml -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -e "Error in string conversion" $<.xml
	rm str2int.failed $<.out

str2int64.test : $(OBJDIR)str2int64$(PINTOOL_SUFFIX) $(OBJDIR)$(THREADTEST) str2int64.tested str2int64.failed
	echo "str2int should fail.  Ignore the error."
	-$(PIN) -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	$(PIN_DIFF) $<.out str2int64.reference
	-$(PIN) -error_file $<.xml -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -e "Error in string conversion" $<.xml
	rm str2int64.failed $<.out

str2flt.test : $(OBJDIR)str2flt$(PINTOOL_SUFFIX) $(OBJDIR)$(THREADTEST) str2flt.tested str2flt.failed
	echo "str2flt should fail.  Ignore the error."
	-$(PIN) -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -e "Non-numeric" $<.out
	-$(PIN) -error_file $<.xml -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -e "Non-numeric" $<.xml
	rm str2flt.failed $<.out

toolfetch_sub.test : $(OBJDIR)toolfetch_sub$(PINTOOL_SUFFIX) $(OBJDIR)tool_fetch_app toolfetch_sub.tested toolfetch_sub.failed
	-$(PIN) -t $< -- ./$(OBJDIR)tool_fetch_app >  $<.out 2>&1
	$(PIN_DIFF) $<.out toolfetch_sub.reference
	rm toolfetch_sub.failed  $<.out

args_err.test : $(OBJDIR)args_err$(PINTOOL_SUFFIX) args_err.tested args_err.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -error_file $<.xml -xyzzy -error -logfile $<.log -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep "Testing errors" $<.xml
	rm $<.xml $<.log
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -error_file $<.xml -xyzzy -fatal  -logfile $<.log -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep "Testing errors" $<.xml
	rm $<.xml $<.log
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -error_file $<.xml -xyzzy -assert -logfile $<.log -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep "Testing assertions" $<.xml
	rm $<.xml $<.log
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -error_file $<.xml -falafel -logfile $<.log -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep "unknown option" $<.xml
	rm $<.xml $<.log
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -error_file $<.xml  -logfile $<.log -t $< >  $<.out 2>&1
	grep "Missing application name" $<.xml
	rm $<.xml $<.log
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -error_file $<.xml  -logfile $<.log -t $< -- no_file >  $<.out 2>&1
	grep "no_file" $<.xml
	rm $<.xml $<.log $<.out
	echo "args_err should fail.  Ignore the error."
	-$(PIN) -- no_file >  $<.out 2>&1
	grep "no_file" $<.out
	rm args_err.failed $<.out

#write_user_error is a negative test.
write_user_error.test : $(OBJDIR)write_user_error$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO2) write_user_error.tested write_user_error.failed
	-$(PIN) -error_file $<.xml  -t $< -- ./$(OBJDIR)$(SIMPLEFOO2) >  $<.out 2>&1
	echo "write_user_error should fail.  Ignore the error."
	grep -e "user specified error message" $<.xml
	grep "arg2" $<.xml
	rm write_user_error.failed $<.out $<.xml

args_lineno.test : $(OBJDIR)args_lineno$(PINTOOL_SUFFIX) args_lineno.tested args_lineno.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	-$(PIN) -logfile $<.log -xyzzy -mesgon log_instrument -t $< -- $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -q -e "args_lineno.cpp" $<.log
	rm $<.out $<.log args_lineno.failed

callapp0.test : $(OBJDIR)callapp0$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO) callapp0.tested callapp0.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	rm callapp0.failed $<.out

callapp0i.test : $(OBJDIR)callapp0i$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO) callapp0i.tested callapp0i.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO) >  $<.out 2>&1
	grep -q -e  "Hello from Bar" $<.out
	rm callapp0i.failed $<.out

callapp1.test : $(OBJDIR)callapp1$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO1) callapp1.tested callapp1.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO1) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "one = 1" $<.out
	rm callapp1.failed $<.out

callapp1i.test : $(OBJDIR)callapp1i$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO1) callapp1i.tested callapp1i.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO1) >  $<.out 2>&1
	grep -q -e  "Hello from Bar" $<.out
	grep -q -e "one = 1" $<.out
	rm callapp1i.failed $<.out

callapp2.test : $(OBJDIR)callapp2$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO2) callapp2.tested callapp2.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO2) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp2.failed $<.out

callapp3.test : $(OBJDIR)callapp3$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO3) callapp3.tested callapp3.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO3) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp3.failed $<.out

callapp4.test : $(OBJDIR)callapp4$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO4) callapp4.tested callapp4.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO4) >  $<.out 2>&1
	grep -q "Bar" $<.out
	grep -q -e "cafe00" $<.out
	rm callapp4.failed $<.out

callapp5.test : $(OBJDIR)callapp5$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO2) callapp5.tested callapp5.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO2) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp5.failed $<.out

callapp6.test : $(OBJDIR)callapp6$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO6) callapp6.tested callapp6.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO6) >  $<.out 2>&1
	grep -q "Blue6" $<.out
	grep -q "Bar6" $<.out
	grep -q "myBar" $<.out
	grep -q "myBlue" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp6.failed $<.out

callapp7.test : $(OBJDIR)callapp7$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO7) callapp7.tested callapp7.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO7) >  $<.out 2>&1
	grep -q "Blue7: twice" $<.out
	grep -q "Bar7: twice" $<.out
	grep -q "myBar" $<.out
	grep -q "myBlue" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp7.failed $<.out

callapp8.test : $(OBJDIR)callapp8$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO8) callapp8.tested callapp8.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO8) >  $<.out 2>&1
	grep -q "Bar8: thrice" $<.out
	grep -q "myBar" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp8.failed $<.out

callapp9.test : $(OBJDIR)callapp9$(PINTOOL_SUFFIX) $(OBJDIR)$(INNER) callapp9.tested callapp9.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(INNER) >  $<.out 2>&1
	test `grep -e "Inner G0=0" $<.out | wc -l` -eq "2"
	rm callapp9.failed $<.out

callapp10.test : $(OBJDIR)callapp10$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO10) callapp10.tested callapp10.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO10) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "one = 1" $<.out
	grep -q -e "ret = 45" $<.out
	rm $<.out
	$(PIN) $(MT) -t $< -- ./$(OBJDIR)$(SIMPLEFOO10) >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "one = 1" $<.out
	grep -q -e "ret = 45" $<.out
	rm callapp10.failed $<.out

callapp10i.test : $(OBJDIR)callapp10i$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO10) callapp10i.tested callapp10i.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO10) >  $<.out 2>&1
	grep -q -e  "Hello from Bar" $<.out
	grep -q -e "one = 1" $<.out
	grep -q -e "ret = 45" $<.out
	rm callapp10i.failed $<.out

callappfast10.test : $(OBJDIR)callappfast10$(PINTOOL_SUFFIX) $(OBJDIR)winfast10 callappfast10.tested callappfast10.failed
	$(PIN) -t $< -- ./$(OBJDIR)winfast10 >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "one = 1" $<.out
	grep -q -e "ret = 45" $<.out
	rm callappfast10.failed $<.out

callappstd10.test : $(OBJDIR)callappstd10$(PINTOOL_SUFFIX) $(OBJDIR)winstd10 callappstd10.tested callappstd10.failed
	$(PIN) -t $< -- ./$(OBJDIR)winstd10 >  $<.out 2>&1
	grep -q "Blue" $<.out
	grep -q -v "Bar" $<.out
	grep -q -e "one = 1" $<.out
	grep -q -e "ret = 45" $<.out
	rm callappstd10.failed $<.out

callapp12.test : $(OBJDIR)callapp12$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO12) callapp12.tested callapp12.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO12) >  $<.out 2>&1
	grep -q "passed" $<.out
	grep -q "Correct" $<.out
	rm callapp12.failed $<.out

callapp13.test : $(OBJDIR)callapp13$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO13) callapp13.tested callapp13.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO13) >  $<.out 2>&1
	grep -q "passed" $<.out
	grep -q "Correct" $<.out
	rm callapp13.failed $<.out

callapp14.test : $(OBJDIR)callapp14$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO14) callapp14.tested callapp14.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO14) >  $<.out 2>&1
	rm callapp14.failed $<.out

# on windows, you must have the correct version of dbghelp.dll in $PIN_HOME\intel64\bin
# for this test to replace the functions.  sanity doesn't, so this is not tested on windows.
callapp15.test : $(OBJDIR)callapp15$(PINTOOL_SUFFIX) $(OBJDIR)$(SIMPLEFOO9) callapp15.tested callapp15.failed
	$(PIN) -t $< -- ./$(OBJDIR)$(SIMPLEFOO9) >  $<.out 2>&1
	grep -q "Blue6" $<.out
	grep -q "Bar6" $<.out
	grep -q "myBlue" $<.out
	grep -q -e "res = 3" $<.out
	rm callapp15.failed $<.out

change_syscall.test : $(OBJDIR)change_syscall$(PINTOOL_SUFFIX) $(OBJDIR)change_syscall_app change_syscall.tested change_syscall.failed
	$(PIN) -t $< -- ./$(OBJDIR)change_syscall_app
	rm change_syscall.failed

$(OBJDIR)inner: inner.c 
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)inner inner.c -g 

$(OBJDIR)inner.exe: inner.c 
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ inner.c 

$(OBJDIR)simple: simple.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)simple simple.c

$(OBJDIR)set_fp_context_xmm_regs_app.exe: set_fp_context_xmm_regs_app.cpp $(SSE-REF_ASM_OBJ) $(THREAD_LIB)
	${CXX} $(APP_CXXFLAGS2) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SSE-REF_ASM_OBJ) $(THREAD_LIB)

$(OBJDIR)set_fp_context_xmm_regs_app: set_fp_context_xmm_regs_app.cpp  $(THREAD_LIB)
	${CXX} $(APP_CXXFLAGS2) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD) -g

$(OBJDIR)simplefoo1: simplefoo1.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo1 simplefoo1.c simplebar.c  -g

$(OBJDIR)simplefoo2: simplefoo2.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo2 simplefoo2.c simplebar.c  -g

$(OBJDIR)simplefoo3: simplefoo3.c simplebar.c simplebar64.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo3 simplefoo3.c simplebar.c simplebar64.c  -g

$(OBJDIR)simplefoo4: simplefoo4.c simplebar.c simplebar64.c
	$(CC) $(APP_CXXFLAGS) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo4 simplefoo4.c simplebar.c simplebar64.c -g

$(OBJDIR)simplefoo6: simplefoo6.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo6 simplefoo6.c simplebar.c  -g

$(OBJDIR)simplefoo7: simplefoo7.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo7 simplefoo7.c simplebar.c  -g

$(OBJDIR)simplefoo8: simplefoo8.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo8 simplefoo8.c simplebar.c  -g

$(OBJDIR)simplefoo9: simplefoo9.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo9 simplefoo9.c simplebar.c  -g

$(OBJDIR)simplefoo10: simplefoo10.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo10 simplefoo10.c simplebar.c  -g

$(OBJDIR)simplefoo12: simplefoo12.c simplebar.c
	$(CC) $(APP_CXXFLAGS) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo12 simplefoo12.c simplebar.c  -g

$(OBJDIR)simplefoo13: simplefoo13.c simplebar.c simplebar64.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo13 simplefoo13.c simplebar.c simplebar64.c  -g

$(OBJDIR)simplefoo14: simplefoo14.c 
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $(OBJDIR)simplefoo14 simplefoo14.c  -g

$(OBJDIR)simplefoo: simplefoo.c simplebar.c $(OBJDIR)simplesp.o
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -static -o $(OBJDIR)simplefoo simplefoo.c simplebar.c $(OBJDIR)simplesp.o -g

$(OBJDIR)simplesp.o: simplesp.asm 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $(OUTOPT)$@ $<

$(OBJDIR)thread_wait: thread_wait.c addit.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ thread_wait.c addit.c -g $(APP_PTHREAD)

$(OBJDIR)little_malloc: little_malloc.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ little_malloc.c -g

$(OBJDIR)little_malloc.exe: little_malloc.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ little_malloc.c

$(OBJDIR)fp_replace_app: fp_replace_app.c
	$(CC) $(APP_CXXFLAGS) $(PIN_LPATHS) -I../Include -I. -o $@ fp_replace_app.c -g

$(OBJDIR)winfoo1: simplefoo1.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo1.c simplebar.c

$(OBJDIR)winfoo2: simplefoo2.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo2.c simplebar.c

$(OBJDIR)winfoo3: simplefoo3.c simplebar.c simplebar64.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ $< simplefoo3.c simplebar.c simplebar64.c

$(OBJDIR)winfoo4: simplefoo4.c simplebar.c simplebar64.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo4.c simplebar.c simplebar64.c

$(OBJDIR)winfoo5: simplefoo5.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo5.c simplebar.c

$(OBJDIR)winfoo6: simplefoo6.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo6.c simplebar.c

$(OBJDIR)winfoo7: simplefoo7.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo7.c simplebar.c

$(OBJDIR)winfoo8: simplefoo8.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo8.c simplebar.c

$(OBJDIR)winfoo9: simplefoo9.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo9.c simplebar.c

$(OBJDIR)winfoo10: simplefoo10.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo10.c simplebar.c

$(OBJDIR)winfast10: simplefast10.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) /Zi ${OUTEXE}$@ simplefast10.c simplebar.c

$(OBJDIR)winstd10: simplestd10.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) /Zi ${OUTEXE}$@ simplestd10.c simplebar.c

$(OBJDIR)winfoo12: simplefoo12.c simplebar.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo12.c simplebar.c

$(OBJDIR)winfoo13: simplefoo13.c simplebar.c simplebar64.c 
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo13.c simplebar.c simplebar64.c

$(OBJDIR)winfoo14: simplefoo14.c 
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ simplefoo14.c 

$(OBJDIR)datasym_app.exe: datasym_app.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) ${OUTEXE}$@ datasym_app.c

$(OBJDIR)datasym_appdbg.exe: datasym_app.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) /Z7 ${OUTEXE}$@ datasym_app.c


$(OBJDIR)threadtestwin.exe: threadtestwin.c addit.c
	$(CC) $(NO_OPTIMIZE) $(NO_LOGO) $(APP_CXXFLAGS) $(DBG) ${OUTEXE}$@ threadtestwin.c addit.c

$(OBJDIR)tmtest : tmtest.cc
	$(CXX) $(APP_CXXFLAGS) ${OUTOPT}$@ tmtest.cc $(APP_PTHREAD)

transactionalmem.test : $(OBJDIR)transactionalmem$(PINTOOL_SUFFIX) transactionalmem.tested transactionalmem.failed tmtest
	$(PIN) -t $< -- ./tmtest
	rm transactionalmem.failed

sse-ref.test : $(OBJDIR)sse-ref$(PINTOOL_SUFFIX) $(OBJDIR)sse sse-ref.tested sse-ref.failed
	$(PIN)  -t $< -- ./$(OBJDIR)sse 5 > $<.out
	diff --ignore-space-change sse-ref.reference $<.out
	rm sse-ref.failed $<.out

# Same as sse-ref.test, but enforces SMC checks in all regions
smc_sse.test : $(OBJDIR)sse-ref$(PINTOOL_SUFFIX) $(OBJDIR)sse $(SSE-REF_ASM_OBJ) smc_sse.tested smc_sse.failed
	$(PIN) -xyzzy -smc_check_all 1 -t $< -- ./$(OBJDIR)sse 5 > $(OBJDIR)smc_sse.out
	diff --ignore-space-change sse-ref.reference $(OBJDIR)smc_sse.out
	rm smc_sse.failed $(OBJDIR)smc_sse.out

test_ip_access.test : $(OBJDIR)test_ip_access$(PINTOOL_SUFFIX) $(OBJDIR)test_ip_access_app  test_ip_access.tested test_ip_access.failed
	$(PIN)  -t $< -- ./$(OBJDIR)test_ip_access_app  > $<.out
	grep   "SUCCESS" $<.out
	rm test_ip_access.failed $<.out	

int3.test: $(OBJDIR)int3del$(PINTOOL_SUFFIX) $(OBJDIR)int3test int3.tested int3.failed
	$(PIN)  -t $< -- ./$(OBJDIR)int3test > $<.out
	rm int3.failed $<.out

# Assembler files for sse-ref
$(OBJDIR)sse-ref_ia32.obj: sse-ref_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)sse-ref_ia32e.obj: sse-ref_ia32e.asm
	ml64 /nologo /c /Fo$@ $<


$(OBJDIR)inline_opt_test_ia32.obj: inline_opt_test_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)inline_opt_test_df_ia32.obj: inline_opt_test_df_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)inline_opt_test_intel64.obj: inline_opt_test_intel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)inline_opt_test_ia32.o: inline_opt_test_ia32.s 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)inline_opt_test_intel64.o: inline_opt_test_intel64.s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)inline_opt_test_app_ia32.obj: inline_opt_test_app_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)inline_opt_test_df_app_ia32.obj: inline_opt_test_df_app_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)inline_opt_test_app_intel64.obj: inline_opt_test_app_intel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)inline_opt_test_app_ia32.o: inline_opt_test_app_ia32.s 
	$(CC) $(APP_CXXFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)inline_opt_test_app_intel64.o: inline_opt_test_app_intel64.s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)inline_opt_test_intel64.obj: inline_opt_test_intel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)set_xmm_scratches_asmIntel64.obj: set_xmm_scratches_asmIntel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)segmented_ea_verifier_win1_ia32e_ms.obj: segmented_ea_verifier_win1_ia32e_ms.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)ea_verifier_addr16_ia32e_ms.obj: ea_verifier_addr16_ia32e_ms.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)analysis_flag_overwrite_tool1_ms.obj: analysis_flag_overwrite_tool1_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)analysis_flag_overwrite_tool2_ms.obj: analysis_flag_overwrite_tool2_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)analysis_flag_overwrite_tool3_ms.obj: analysis_flag_overwrite_tool3_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)flag_spill_fill_tool1_ms.obj: flag_spill_fill_tool1_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)df_test_tool1_ms.obj: df_test_tool1_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)df_test_tool2_ms.obj: df_test_tool2_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)df_test_tool3_ms.obj: df_test_tool3_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)df_test_tool4_ms.obj: df_test_tool4_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)df_test_tool5_ms.obj: df_test_tool5_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)inlined-stack-arg2_ms.obj: inlined-stack-arg2_ms.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)mxcsr_ia32.obj: mxcsr_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)mxcsr_intel64.obj: mxcsr_intel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)mxcsr_intel64.o: mxcsr_intel64.s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)mxcsr_ia32.o: mxcsr_ia32.s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<


$(OBJDIR)supports_avx_ia32.obj: supports_avx_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)supports_avx_intel64.obj: supports_avx_intel64.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)supports_avx_intel64.o: supports_avx_intel64.s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)supports_avx_ia32.o: supports_avx_ia32.s
	$(CC) $(COPT) $(APP_CXXFLAGS) ${OUTOPT}$@ $<

xmmtest.test : $(OBJDIR)xmmtest$(PINTOOL_SUFFIX) $(OBJDIR)xmmapp$(EXEEXT) xmmtest.tested xmmtest.failed
	$(PIN)  -t $< -- ./$(OBJDIR)xmmapp$(EXEEXT) 9 > $<.out
	./$(OBJDIR)xmmapp$(EXEEXT) 9 > xmmapp.out
	diff --ignore-space-change xmmapp.out $<.out
	rm xmmtest.failed xmmapp.out $<.out

$(OBJDIR)swizzlealloc_ia32.obj: swizzlealloc_ia32.asm
	ml /nologo /c /Fo$@ $< 

# A test program that uses SSE 
$(OBJDIR)sse: sse.cpp $(SSE-REF_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS2) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SSE-REF_ASM_OBJ)

$(OBJDIR)sse_ia32.obj: sse_ia32.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)sse_ia32e.obj: sse_ia32e.asm
	ml64 /nologo /c /Fo$@ $<

$(OBJDIR)test_ip_access_app: test_ip_access_app.cpp $(TEST_IP_ACCESS_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(TEST_IP_ACCESS_ASM_OBJ)

$(OBJDIR)test_ip_access_app_ia32e.obj: test_ip_access_app_ia32e.asm
	ml64 /nologo /c /Fo$@ $<

# A test program that for xmm test 
$(OBJDIR)xmmapp.$(OBJEXT): xmmapp.cpp
	${CXX} $(APP_CXXFLAGS) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTOPT}$@ $(COPT) $<

$(OBJDIR)xmmapp$(EXEEXT): $(OBJDIR)xmmapp.$(OBJEXT) $(SSE-REF_ASM_OBJ) 
	${CXX} $(APP_CXXFLAGS) ${OUTEXE}$@ $< $(SSE-REF_ASM_OBJ) 


$(OBJDIR)inline_opt_test_app.$(OBJEXT): inline_opt_test_app.cpp
	${CXX} $(APP_CXXFLAGS) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTOPT}$@ $(COPT) $<

$(OBJDIR)inline_opt_test_app$(EXEEXT): $(OBJDIR)inline_opt_test_app.$(OBJEXT) $(SSE-REF_ASM_OBJ) $(OBJDIR)$(INLINE_OPT_TEST_ASM_OBJ) $(OBJDIR)$(INLINE_OPT_TEST_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) ${OUTEXE}$@ $< $(SSE-REF_ASM_OBJ) $(OBJDIR)$(INLINE_OPT_TEST_ASM_OBJ) $(OBJDIR)$(INLINE_OPT_TEST_APP_ASM_OBJ)

$(OBJDIR)inline_opt_test_df_app.$(OBJEXT): inline_opt_test_df_app.cpp
	${CXX} $(APP_CXXFLAGS) $(DBG) $(NO_OPTIMIZE) $(SSE2) ${OUTOPT}$@ $(COPT) $<

$(OBJDIR)inline_opt_test_df_app$(EXEEXT): $(OBJDIR)inline_opt_test_df_app.$(OBJEXT)   $(OBJDIR)$(INLINE_OPT_TEST_DF_APP_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) ${OUTEXE}$@ $<  $(OBJDIR)$(INLINE_OPT_TEST_DF_APP_ASM_OBJ)

branch_target_addr.test : %.test : $(OBJDIR)branch_target_addr$(PINTOOL_SUFFIX) $(OBJDIR)doint_ia32 %.tested %.failed
	$(PIN)  -t $< -- ./$(OBJDIR)doint_ia32
	rm branch_target_addr.failed

$(OBJDIR)doint_ia32: doint.c doint_ia32.s
	$(CC) $(APP_CXXFLAGS) -o $@ doint.c -x assembler-with-cpp doint_ia32.s

mmap.test: $(OBJDIR)mmap$(PINTOOL_SUFFIX) $(OBJDIR)mmapapp mmap.tested mmap.failed
	$(PIN) -t $(OBJDIR)mmap$(PINTOOL_SUFFIX) -- ./$(OBJDIR)mmapapp
	rm -f mmap.failed

$(OBJDIR)mmapapp: mmapapp.c
	$(CC) $(APP_CXXFLAGS) ${OUTEXE}$@ mmapapp.c

$(OBJDIR)change_syscall_app: change_syscall_app.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ change_syscall_app.c

pipe.test: $(OBJDIR)pipeapp pipe.tested pipe.failed
	$(PIN) -- ./$(OBJDIR)pipeapp
	rm -f pipe.failed

$(OBJDIR)pipeapp: pipeapp.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ pipeapp.c

smc_ipf.test: $(OBJDIR)smcapp_ipf smc_ipf.tested smc_ipf.failed
	$(PIN) -- ./$(OBJDIR)smcapp_ipf > $<.out 2>&1
	$(PIN_DIFF) $<.out smc_ipf.reference
	rm smc_ipf.failed $<.out

smc_ia32.test: $(OBJDIR)smcapp_ia32 smc_ia32.tested smc_ia32.failed
	$(PIN) -- ./$(OBJDIR)smcapp_ia32
	rm smc_ia32.failed

smc_except.test: $(OBJDIR)smcapp_except smc_except.tested smc_except.failed
	$(PIN) -- ./$(OBJDIR)smcapp_except
	rm smc_except.failed

smc_mt.test: $(OBJDIR)smcapp_mt smc_mt.tested smc_mt.failed
	$(PIN) -- ./$(OBJDIR)smcapp_mt
	rm smc_mt.failed

smc_bbl.test: $(OBJDIR)smcapp_bbl smc_bbl.tested smc_bbl.failed
	$(PIN) -smc_strict -- ./$(OBJDIR)smcapp_bbl
	rm smc_bbl.failed

$(OBJDIR)smcapp_ipf: smcapp_ipf.c smcapp_ipf_asm.s
	$(CC) ${OUTOPT}$@ smcapp_ipf.c -x assembler-with-cpp smcapp_ipf_asm.s

attachnat.test: $(OBJDIR)attachnat attachnat.tested attachnat.failed
	rm -f attachnat.pid; \
		./$(OBJDIR)attachnat 2> attachnat.pid 1> attachnat.out & \
		until test -s attachnat.pid; \
			do sleep 1; \
		done; \
		pid=`head -1 attachnat.pid | sed 's/Attach to me: //'`; \
		$(PIN) -pid $$pid; \
		wait $$!
	$(PIN_DIFF) attachnat.out attachnat.reference
	rm attachnat.failed attachnat.out attachnat.pid

$(OBJDIR)attachnat: attachnat.c attachnat_asm.s
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ attachnat.c -x assembler-with-cpp attachnat_asm.s

attach.test: %.test: $(OBJDIR)attach_app  $(OBJDIR)attach$(PINTOOL_SUFFIX) %.tested %.failed
	touch attach_tool.out
	$< -pin $(PIN) -pinarg -t $(OBJDIR)attach$(PINTOOL_SUFFIX) -o attach_tool.out > $*.err 2>&1
	$(PIN_DIFF) attach_tool.out attach_tool.reference || grep "THREAD_AREA" $*.err
	rm $*.failed attach_tool.out $*.err

$(OBJDIR)attach_app: attach_app.cpp
	$(CXX) --no-inline $(APP_CXXFLAGS) ${OUTEXE}$@ $<

spalign.test : $(OBJDIR)spalign$(PINTOOL_SUFFIX) spalign.tested spalign.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	rm spalign.failed $<.makefile.copy

spalign_no_inline_bridge.test : $(OBJDIR)spalign$(PINTOOL_SUFFIX) spalign_no_inline_bridge.tested spalign_no_inline_bridge.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline_bridge 0 -t $< -- $(TESTAPP) makefile spalign_no_inline_bridge.makefile.copy
	rm spalign_no_inline_bridge.failed spalign_no_inline_bridge.makefile.copy


$(OBJDIR)spalign_asm_ia32e_ms.obj : spalign_asm_ia32e_ms.asm
	ml64 /nologo /c /Fo$@ $< 

$(OBJDIR)spalign$(PINTOOL_SUFFIX): $(OBJDIR)spalign.o $(SP_ALIGN_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) $(SP_ALIGN_ASM_OBJ)

$(OBJDIR)exception_monitor_for_iret_order$(PINTOOL_SUFFIX): $(OBJDIR)exception_monitor_for_iret_order.o 
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)insertand$(PINTOOL_SUFFIX): $(OBJDIR)insertand.o $(OBJDIR)and.o
	${PIN_LD} $(OBJDIR)and.o $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)clobber$(PINTOOL_SUFFIX): $(OBJDIR)clobber.o $(OBJDIR)clobber_asm.o
	${PIN_LD} $(OBJDIR)clobber_asm.o $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)lpd: lpd.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)lpd lpd.c

clobber_xmm.test : $(OBJDIR)clobber$(PINTOOL_SUFFIX) clobber_xmm.tested clobber_xmm.failed $(OBJDIR)lpd
	$(PIN) -t $< -- ./$(OBJDIR)lpd
	rm clobber_xmm.failed

incebx.test : $(OBJDIR)incebx$(PINTOOL_SUFFIX) incebx.tested incebx.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	rm incebx.failed

badpath.test : badpath.tested badpath.failed
	-$(PIN) -- $(OBJDIR)BadAPPpAth >  badpath.out 2>&1
	grep "No such file" badpath.out 
	-$(PIN) -error_file $<.xml  -logfile $<.log -- $(OBJDIR)BadAPPpAth >  badpath.out 2>&1
	grep "No such file" $<.xml 
	rm badpath.failed badpath.out $<.xml $<.log

badfile.test : %.test: %.tested %.failed
	-$(PIN) -error_file $*.xml -logfile $*.log -- ./makefile >  $*.out 2>&1 || echo "non-zero status" >> $*.log
	grep -e "Permission denied" $*.log
	grep -q non-zero $*.log
	rm $*.failed $*.out $*.log $*.xml

$(OBJDIR)cancel: cancel.c
	$(CC) $(APP_CXXFLAGS) -o $@ $< $(APP_PTHREAD)

cancel.test: %.test: $(OBJDIR)% %.tested %.failed
	-./$(OBJDIR)cancel > cancel.native.out 2>&1
	-$(PIN) -- ./$(OBJDIR)cancel > cancel.pin.out 2>&1
	cmp cancel.native.out cancel.pin.out
	rm cancel.failed

$(OBJDIR)xmmtest.o: xmmtest.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) $(SSE2) ${OUTOPT}$@ $<

$(OBJDIR)safecopy_mxcsr.o: safecopy_mxcsr.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) $(SSE2) ${OUTOPT}$@ $<

$(OBJDIR)context_state_apis.o: context_state_apis.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) $(SSE2) ${OUTOPT}$@ $<

thread_count.test : %.test: $(OBJDIR)%$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)$(HELLO)
	touch hello.out; rm hello.out
	touch thread_count.out; rm thread_count.out
	$(PIN) -t $< -- $(OBJDIR)$(HELLO) >  hello.out 2>&1
	$(PIN_DIFF) hello.out hello.reference
	$(PIN_DIFF) thread_count.out thread_count.reference
	rm thread_count.failed thread_count.out hello.out

internal_exception_handler.test: %.test: $(OBJDIR)%$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)%_app
	$(PIN) -t $< -- ./$(OBJDIR)internal_exception_handler_app > $*.out	
	test `grep "GlobalHandler2: Caught exception. Exception Code: RECEIVED_ACCESS_FAULT." $*.out | wc -l` -eq "3"
	test `grep "GlobalHandler1: Caught exception. Exception Code: RECEIVED_ACCESS_FAULT." $*.out | wc -l` -eq "3"
	test `grep "GlobalHandler1: Fixing exception and continuing execution" $*.out | wc -l` -eq "3"
	test `grep "AnalysisHandler2: Caught exception. Exception Code: INT_DIVIDE_BY_ZERO." $*.out | wc -l` -eq "1"
	test `grep "AnalysisHandler2: Matching exception address. Continue search" $*.out | wc -l` -eq "1"
	test `grep "AnalysisHandler1: Caught exception. Exception Code: INT_DIVIDE_BY_ZERO." $*.out | wc -l` -eq "1"
	test `grep "AnalysisHandler1: Raising exception on behalf of the application" $*.out | wc -l` -eq "2"
	test `grep "AnalysisHandler1: Caught exception. Exception Code: X87_DIVIDE_BY_ZERO." $*.out | wc -l` -eq "1"
	test `grep "internal_exception_handler : Completed successfully" $*.out | wc -l` -eq "1"
	rm internal_exception_handler.failed internal_exception_handler.out

internal_exception_handler_lin.test: %.test: $(OBJDIR)%$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)internal_exception_handler_app_lin
	$(PIN) -t $< -- ./$(OBJDIR)internal_exception_handler_app_lin > $*.out	
	test `grep "Fixing ACCESS_DENIED" $*.out | wc -l` -eq "3"
	test `grep "DIVIDE_BY_ZERO" $*.out | grep "Fixing" | wc -l` -eq "2"
	test `grep "Fixing ACCESS_INVALID_ADDRESS" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out
         
thread_count_thread_creation.test : %.test: $(OBJDIR)thread_count$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_thread_create_apc
	touch thread_count.out; rm thread_count.out
	touch win_thread_create_apc.out; rm win_thread_create_apc.out
	mkdir -p $(OBJDIR)"I Need My Space"
	$(PIN) -xyzzy -logfile $(OBJDIR)"I Need My Space/pin.log" -mesgon log_win -t $< -- $(OBJDIR)win_thread_create_apc >  win_thread_create_apc.out 2>&1
	$(PIN_DIFF) win_thread_create_apc.out win_thread_create_apc.reference
	$(PIN_DIFF) thread_count.out thread_count_thread_creation.reference
	rm thread_count_thread_creation.failed thread_count.out win_thread_create_apc.out $(OBJDIR)"I Need My Space/pin.log"
	rmdir $(OBJDIR)"I Need My Space"

win_multi_dll_pintool.test : %.test: $(OBJDIR)win_multi_dll_pintool %.tested %.failed $(OBJDIR)win_thread_create_apc
	touch main_dll.out; rm main_dll.out
	touch static_secondary_dll.out; rm static_secondary_dll.out
	touch dynamic_secondary_dll.out; rm dynamic_secondary_dll.out
	touch win_thread_create_apc.out; rm win_thread_create_apc.out
	$(PIN) -t main_dll -- $(OBJDIR)win_thread_create_apc >  win_thread_create_apc.out 2>&1
	$(PIN_DIFF) win_thread_create_apc.out win_thread_create_apc.reference
	$(PIN_DIFF) static_secondary_dll.out dynamic_secondary_dll.out
	rm $*.failed static_secondary_dll.out dynamic_secondary_dll.out win_thread_create_apc.out

thread_creation_late_injection.test : %.test: %.tested %.failed $(OBJDIR)win_thread_create_apc
	touch thread_creation_late_injection.out; rm thread_creation_late_injection.out
	$(PIN) -xyzzy -late_injection -- $(OBJDIR)win_thread_create_apc
	rm $*.failed

reg_inst_gx.test : $(OBJDIR)reg_inst_gx$(PINTOOL_SUFFIX) reg_inst_gx.tested reg_inst_gx.failed $(OBJDIR)thread
	$(PIN) -t $< -o $(@:.test=.tool.out) -- ./$(OBJDIR)thread > $(@:.test=.out) 2>&1
	$(PIN_DIFF) $(@:.test=.out) $(@:.test=.reference)
	$(PIN_DIFF) $(@:.test=.tool.out) $(@:.test=.tool.reference)
	rm $(@:.test=.failed)

win_queue_apc.test : %.test: $(OBJDIR)apc_monitor$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_queue_apc
	touch apc_monitor.out; rm apc_monitor.out
	touch win_queue_apc.out; rm win_queue_apc.out
	$(PIN) -t $< -- $(OBJDIR)win_queue_apc >  win_queue_apc.out 2>&1
	$(PIN_DIFF) win_queue_apc.out win_queue_apc.reference
	$(PIN_DIFF) apc_monitor.out apc_monitor.reference 
	rm win_queue_apc.failed apc_monitor.out win_queue_apc.out

win_divide_by_zero_exception.test : %.test: $(OBJDIR)exception_monitor$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_divide_by_zero_exception
	touch win_divide_by_zero_exception.out; rm win_divide_by_zero_exception.out
	touch exception_monitor.out; rm exception_monitor.out
	$(PIN) -t $< -- $(OBJDIR)win_divide_by_zero_exception >  win_divide_by_zero_exception.out 2>&1
	$(PIN_DIFF) win_divide_by_zero_exception.out win_divide_by_zero_exception.reference
	$(PIN_DIFF) exception_monitor.out exception_monitor_for_divide_by_zero_test.reference
	rm win_divide_by_zero_exception.failed exception_monitor.out win_divide_by_zero_exception.out

win_divide_by_zero_exception_for_iretq_order.test : %.test: $(OBJDIR)exception_monitor_for_iret_order$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_divide_by_zero_exception
	touch win_divide_by_zero_exception_for_iretq_order.out; rm win_divide_by_zero_exception_for_iretq_order.out
	touch exception_monitor_for_iret_order.out; rm exception_monitor_for_iret_order.out
	$(PIN) -t $< -- $(OBJDIR)win_divide_by_zero_exception >  win_divide_by_zero_exception_for_iretq_order.out 2>&1
	rm win_divide_by_zero_exception_for_iretq_order.failed  win_divide_by_zero_exception_for_iretq_order.out

win_cpp_exception.test : $(OBJDIR)exception_monitor$(PINTOOL_SUFFIX) win_cpp_exception.tested win_cpp_exception.failed $(OBJDIR)win_cpp_exception
	touch win_cpp_exception.out; rm win_cpp_exception.out
	touch exception_monitor.out; rm exception_monitor.out
	$(PIN) -t $< -- $(OBJDIR)win_cpp_exception >  win_cpp_exception.out 2>&1
	$(PIN_DIFF) win_cpp_exception.out win_cpp_exception.reference
	$(PIN_DIFF) exception_monitor.out exception_monitor_for_cpp_exception_test.reference
	rm win_cpp_exception.failed exception_monitor.out win_cpp_exception.out

win_debug_service.test : %.test: $(OBJDIR)debugservice_monitor$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_debug_service
	touch debugservice_monitor.out; rm debugservice_monitor.out
	touch win_debug_service.out; rm win_debug_service.out
	$(PIN) -t $< -- $(OBJDIR)win_debug_service >  win_debug_service.out 2>&1
	$(PIN_DIFF) win_debug_service.out win_debug_service.reference
	$(PIN_DIFF) debugservice_monitor.out debugservice_monitor.reference
	rm win_debug_service.failed debugservice_monitor.out win_debug_service.out

win_load_library.test : %.test: $(OBJDIR)thread_count$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_load_library
	touch thread_count.out; rm thread_count.out
	touch win_load_library.out; rm win_load_library.out
	$(PIN) -t $< -- $(OBJDIR)win_load_library >  win_load_library.out 2>&1
	$(PIN_DIFF) win_load_library.out win_load_library.reference
	$(PIN_DIFF) thread_count.out thread_count_load_library.reference
	rm win_load_library.failed thread_count.out win_load_library.out

win_code_on_reused_memory.test : %.test : %.tested %.failed $(OBJDIR)win_code_on_reused_memory
	touch win_code_on_reused_memory.out; rm win_code_on_reused_memory.out
	$(PIN) -- $(OBJDIR)win_code_on_reused_memory >  win_code_on_reused_memory.out 2>&1
	$(PIN_DIFF) win_code_on_reused_memory.out win_code_on_reused_memory.reference
	rm win_code_on_reused_memory.failed win_code_on_reused_memory.out	

win_exception_context.test: %.test: $(OBJDIR)winapp_exception_context $(OBJDIR)exception_context_monitor$(PINTOOL_SUFFIX) %.tested %.failed 
	$(PIN) -t $(OBJDIR)exception_context_monitor -checkfp 1 -- ./$(OBJDIR)winapp_exception_context
	rm win_exception_context.failed

win_exception_detect.test: %.test: $(OBJDIR)win_exception_test $(OBJDIR)exception_context_monitor$(PINTOOL_SUFFIX) %.tested %.failed 
	$(PIN) -t $(OBJDIR)exception_context_monitor -checkfp 1 -- ./$(OBJDIR)win_exception_test
	rm win_exception_detect.failed

win_mxcsr.test : %.test: $(OBJDIR)thread_count$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_mxcsr_app
	touch thread_count.out; rm thread_count.out
	touch win_mxcsr_app.out; rm win_mxcsr_app.out
	touch win_mxcsr_app_with_pin.out; rm win_mxcsr_app_with_pin.out
	$(OBJDIR)win_mxcsr_app > win_mxcsr_app.out 2>&1
	$(PIN) -t $< -- $(OBJDIR)win_mxcsr_app > win_mxcsr_app_with_pin.out 2>&1
	$(PIN_DIFF) win_mxcsr_app.out win_mxcsr_app_with_pin.out
	rm win_mxcsr.failed thread_count.out win_mxcsr_app.out win_mxcsr_app_with_pin.out

win_hello_c_sharp.test : %.test: %.tested %.failed $(OBJDIR)win_hello_c_sharp_app.exe
	touch win_hello_c_sharp.out; rm win_hello_c_sharp.out
	$(PIN) -- $(OBJDIR)win_hello_c_sharp_app.exe > win_hello_c_sharp.out 2>&1
	grep -q "Hello world!"  win_hello_c_sharp.out 
	rm win_hello_c_sharp.out
	touch win_hello_c_sharp.out; rm win_hello_c_sharp.out
	$(PIN) -xyzzy -late_injection -- $(OBJDIR)win_hello_c_sharp_app.exe > win_hello_c_sharp.out 2>&1
	grep -q "Hello world!"  win_hello_c_sharp.out 
	rm win_hello_c_sharp.failed  win_hello_c_sharp.out	

win_early_injection1.test : %.test: $(OBJDIR)image_load$(PINTOOL_SUFFIX) $(OBJDIR)win_early_thread %.tested %.failed
	touch $*.out; rm $*.out
	$(PIN) -t $< -- $(OBJDIR)win_early_thread >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "1"
	grep "Creating thread in DllMain(PROCESS_ATTACH)" $*.out
	test `grep "Got thread start notification" $*.out | wc -l` -eq "2"
	touch $*.out; rm $*.out

	$(PIN) -t $< -load_system_dlls 1 -- $(OBJDIR)win_early_thread >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "1"
	grep "Creating thread in DllMain(PROCESS_ATTACH)" $*.out
	test `grep "Got thread start notification" $*.out | wc -l` -eq "2"	
	rm $*.failed $*.out	

win_early_injection1_probed.test : %.test: $(OBJDIR)image_load$(PINTOOL_SUFFIX) $(OBJDIR)win_early_thread %.tested %.failed
	touch $*.out; rm $*.out
	$(PIN) -probe -t $< -- $(OBJDIR)win_early_thread >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "1"
	grep "Creating thread in DllMain(PROCESS_ATTACH)" $*.out
	touch $*.out; rm $*.out

	$(PIN) -probe -t $< -load_system_dlls 1 -- $(OBJDIR)win_early_thread >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "1"
	grep "Creating thread in DllMain(PROCESS_ATTACH)" $*.out
	rm $*.failed $*.out

win_early_injection2.test : %.test: $(OBJDIR)image_load$(PINTOOL_SUFFIX) $(OBJDIR)win_no_dependencies %.tested %.failed
	touch $*.out; rm $*.out
	$(PIN) -t $< -- $(OBJDIR)win_no_dependencies >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "1"
	touch $*.out; rm $*.out

	$(PIN) -t $< -load_system_dlls 1 -- $(OBJDIR)win_no_dependencies >$*.out 2>&1
	test `grep "In tool's main, probed = 0" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out	

win_early_injection2_probed.test : %.test: $(OBJDIR)image_load$(PINTOOL_SUFFIX) $(OBJDIR)win_no_dependencies %.tested %.failed
	touch $*.out; rm $*.out
	$(PIN) -probe -t $< -- $(OBJDIR)win_no_dependencies >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "1"
	touch $*.out; rm $*.out

	$(PIN) -probe -t $< -load_system_dlls 1 -- $(OBJDIR)win_no_dependencies >$*.out 2>&1
	test `grep "In tool's main, probed = 1" $*.out | wc -l` -eq "1"
	rm $*.failed $*.out	

image_entry.test : %.test: $(OBJDIR)image_entry$(PINTOOL_SUFFIX) %.tested %.failed $(OBJDIR)win_thread_create_apc
	touch image_entry.out; rm image_entry.out
	touch win_thread_create_apc.out; rm win_thread_create_apc.out
	$(PIN) -probe -t $< -out_file  image_entry_probe.out -- $(OBJDIR)win_thread_create_apc  >  win_thread_create_apc.out 2>&1
	$(PIN_DIFF) win_thread_create_apc.out win_thread_create_apc.reference
	$(PIN_DIFF) image_entry_probe.out image_entry.reference
	rm win_thread_create_apc.out
	$(PIN) -t $< -out_file  image_entry_jit.out -- $(OBJDIR)win_thread_create_apc  >  win_thread_create_apc.out 2>&1
	$(PIN_DIFF) win_thread_create_apc.out win_thread_create_apc.reference
	$(PIN_DIFF) image_entry_jit.out image_entry.reference
	rm win_thread_create_apc.out
	rm image_entry.failed image_entry_probe.out image_entry_jit.out

funreplace_alert.test : $(OBJDIR)funreplace_alert$(PINTOOL_SUFFIX) funreplace_alert.tested funreplace_alert.failed $(OBJDIR)socket_app
	touch funreplace_alert.out; rm funreplace_alert.out
	$(PIN) -t $< -- $(OBJDIR)socket_app >  funreplace_alert.out 2>&1
	$(PIN_DIFF) funreplace_alert.out funreplace_alert.reference
	rm funreplace_alert.failed funreplace_alert.out

syscall_std.test : $(OBJDIR)syscall_std$(PINTOOL_SUFFIX) syscall_std.tested syscall_std.failed $(OBJDIR)syscall_std_app
	touch syscall_std.out; rm syscall_std.out
	$(PIN) -t $< -- $(OBJDIR)syscall_std_app >  syscall_std.out 2>&1
	$(PIN_DIFF) syscall_std.out syscall_std.reference
	rm syscall_std.failed syscall_std.out

teb.test : $(OBJDIR)teb$(PINTOOL_SUFFIX) teb.tested teb.failed $(OBJDIR)teb_app
	touch teb.out; rm teb.out
	$(PIN) -t $< -- $(OBJDIR)teb_app >  teb.out 2>&1
	$(PIN_DIFF) teb.out teb.reference
	rm teb.failed teb.out

earlyout.test: $(OBJDIR)earlyout$(PINTOOL_SUFFIX) earlyout.tested earlyout.failed $(OBJDIR)$(HELLO)
	$(PIN) -t $< -- ./$(OBJDIR)$(HELLO)
	rm earlyout.failed

guard_page.test: $(OBJDIR)guard_page$(PINTOOL_SUFFIX) guard_page.tested guard_page.failed $(OBJDIR)guard_page_app
	$(PIN) -t $< -- ./$(OBJDIR)guard_page_app
	rm guard_page.failed

suspend_win.test: $(OBJDIR)suspend_win$(PINTOOL_SUFFIX) suspend_win.tested suspend_win.failed $(OBJDIR)suspend_app_win
	$(PIN) -t $< -- ./$(OBJDIR)suspend_app_win
	rm suspend_win.failed

suspend_context_win.test: suspend_context_win.tested suspend_context_win.failed $(OBJDIR)suspend_context_app_win
	$(PIN) -- ./$(OBJDIR)suspend_context_app_win
	rm suspend_context_win.failed

raise_exception.test: $(OBJDIR)raise_exception$(PINTOOL_SUFFIX) raise_exception.tested raise_exception.failed $(OBJDIR)raise_exception_app
	$(PIN) -t $< -- ./$(OBJDIR)raise_exception_app
	rm raise_exception.failed

sw_interrupt.test: $(OBJDIR)sw_interrupt$(PINTOOL_SUFFIX) sw_interrupt.tested sw_interrupt.failed $(OBJDIR)sw_interrupt_app
	$(PIN) -t $< -- ./$(OBJDIR)sw_interrupt_app
	rm sw_interrupt.failed

internal_thread.test: $(OBJDIR)mt_tool$(PINTOOL_SUFFIX) internal_thread.tested internal_thread.failed $(OBJDIR)mt_app
	$(PIN) -t $< -- ./$(OBJDIR)mt_app
	rm internal_thread.failed

win_callback.test: $(OBJDIR)win_callback$(PINTOOL_SUFFIX) win_callback.tested win_callback.failed $(OBJDIR)win_callback_app
	$(PIN) -t $< -- ./$(OBJDIR)win_callback_app
	rm win_callback.failed

$(OBJDIR)win_callback_app: win_callback_app.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) user32.lib

$(OBJDIR)mt_app: mt_app.cpp ../threadlib/threadlib.h $(THREAD_LIB)
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(THREAD_LIB) $(APP_PTHREAD)

$(OBJDIR)sw_interrupt_app: sw_interrupt_app.cpp sys_memory.h $(OBJDIR)sys_memory.$(OBJEXT)
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)sys_memory.$(OBJEXT)

$(OBJDIR)raise_exception_app: raise_exception_app.cpp sys_memory.h $(OBJDIR)sys_memory.$(OBJEXT)
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)sys_memory.$(OBJEXT)

$(OBJDIR)suspend_context_app_win: suspend_context_app_win.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)suspend_app_win: suspend_app_win.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)guard_page_app: guard_page_app.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)socket_app: socket_app.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) ws2_32.lib

$(OBJDIR)smcapp_ia32: smcapp_ia32.cpp $(SMC_LIB) smc_util.h sys_memory.h ../threadlib/threadlib.h
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SMC_LIB) $(APP_PTHREAD)

$(OBJDIR)smcapp_except: smcapp_except.cpp $(SMC_LIB) smc_util.h sys_memory.h ../threadlib/threadlib.h
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SMC_LIB) $(APP_PTHREAD)

$(OBJDIR)smcapp_mt: smcapp_mt.cpp $(SMC_LIB) smc_util.h sys_memory.h ../threadlib/threadlib.h
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SMC_LIB) $(APP_PTHREAD)

$(OBJDIR)smcapp_bbl: smcapp_bbl.cpp $(SMC_LIB) smc_util.h sys_memory.h ../threadlib/threadlib.h
	$(CXX)  ${APP_CXXFLAGS} $(OPT) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(SMC_LIB) $(APP_PTHREAD)

$(OBJDIR)syscall_std_app: syscall_std_app.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)teb_app: teb_app.cpp
        #use big stack in application
	$(CXX)  ${APP_CXXFLAGS} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) /STACK:0x800000,0x1000

$(OBJDIR)win_load_library: win_load_library.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)internal_exception_handler_app: internal_exception_handler_app.cpp
	$(CXX) /fp:precise ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)internal_exception_handler_app_lin: internal_exception_handler_app_lin.cpp exception_app_$(TARGET).s
	$(CC) -c ${APP_CXXFLAGS} exception_app_$(TARGET).s -o $(OBJDIR)exception_app_$(TARGET).o
	$(CXX)  $(DBG) ${APP_CXXFLAGS} -o $@ $< $(APP_CXXLINK_FLAGS) -Wl,$(OBJDIR)exception_app_$(TARGET).o
         
$(OBJDIR)win_code_on_reused_memory: win_code_on_reused_memory.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${NO_COMDAT_FLAG} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_thread_create_apc: win_thread_create_apc.cpp $(OBJDIR)win_tls_dll.dll
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)win_tls_dll.lib

$(OBJDIR)win_tls_dll.dll: win_tls_dll.cpp
	$(CXX) $(APP_CXXFLAGS2)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS) /dll

$(OBJDIR)win_queue_apc: win_queue_apc.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_divide_by_zero_exception: win_divide_by_zero_exception.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_cpp_exception: win_cpp_exception.cpp
	$(CXX)  ${APP_CXXFLAGS2} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)winapp_exception_context: winapp_exception_context.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_debug_service: win_debug_service.cpp
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_mxcsr_app: win_mxcsr_app.cpp
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)win_hello_c_sharp_app.exe: win_hello_c_sharp_app_bin
        #csc /optimize+ /out:$@ win_hello_c_sharp_app.cs
	cp win_hello_c_sharp_app_bin $(OBJDIR)win_hello_c_sharp_app.exe

$(OBJDIR)win_early_thread: win_early_thread.cpp $(OBJDIR)win_early_thread_dll.dll
	$(CXX)  ${APP_CXXFLAGS2} ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) Advapi32.lib Dbghelp.lib $(OBJDIR)win_early_thread_dll.lib

$(OBJDIR)win_early_thread_dll.dll: win_early_thread_dll.cpp
	$(CXX) $(APP_CXXFLAGS2)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS) /dll

$(OBJDIR)win_no_dependencies: win_no_dependencies.cpp
	$(CXX)  ${OUTEXE}$@ $< /link /nodefaultlib /entry:main  

$(OBJDIR)rep: rep_$(TARGET).s
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ $< -static -g

$(OBJDIR)big_bss: big_bss.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$@ big_bss.c

$(OBJDIR)sp_argument_app: sp_argument_app.c
	$(CXX)  ${APP_CXXFLAGS} $(NO_OPTIMIZE) ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)btc_app: btc_app.c btc.s
	$(CC) $(APP_CXXFLAGS) -o $@ btc_app.c -x assembler-with-cpp btc.s

sp_argument.test: $(OBJDIR)sp_argument$(PINTOOL_SUFFIX) $(OBJDIR)sp_argument_app sp_argument.tested sp_argument.failed
	$(PIN) -t $< -- ./$(OBJDIR)sp_argument_app
	$(PIN) -probe -t $< -- ./$(OBJDIR)sp_argument_app
	rm sp_argument.failed

reptool.test: $(OBJDIR)reptool$(PINTOOL_SUFFIX) $(OBJDIR)rep reptool.tested reptool.failed
	$(PIN) -t $< -o reptool.out -- ./$(OBJDIR)rep
	$(PIN_DIFF) reptool.out reptool_$(TARGET).reference
	rm reptool.failed

reppredicate.test: $(OBJDIR)checkexecuting$(PINTOOL_SUFFIX) $(OBJDIR)rep reppredicate.tested reppredicate.failed
	$(PIN) -t $< -o reppredicate.out -- ./$(OBJDIR)rep
	$(PIN_DIFF) reppredicate.out reppredicate_$(TARGET).reference
	rm reppredicate.failed

$(OBJDIR)repcmpsz: repcmpsz.s
	$(AS) $(AS_FLAGS) -o $@.o repcmpsz.s
	$(LD) $(ASLD_FLAGS) -o $@ $@.o

$(OBJDIR)repcmpsz.exe: repcmpsz.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)repcmpsz.exe repcmpsz.c

repcmpsz_tool.test: $(OBJDIR)repcmpsz_tool$(PINTOOL_SUFFIX) $(OBJDIR)repcmpsz$(EXEEXT) repcmpsz.tested repcmpsz_tool.failed
	$(PIN) -t $< -- ./$(OBJDIR)repcmpsz
	$(PIN_DIFF) repcmpsz_tool.out repcmpsz_tool.reference
	rm repcmpsz_tool.failed

waitpidbug.test: %.test: $(OBJDIR)waitpidbug %.tested %.failed
	./$< $(PIN_EXE) $(TESTAPP)
	rm $*.failed

$(OBJDIR)waitpidbug: waitpidbug.c
	$(CC) $(APP_CXXFLAGS) -o $(OBJDIR)waitpidbug waitpidbug.c

big_bss.test: big_bss.failed big_bss.tested $(OBJDIR)big_bss
	$(PIN) -- $(OBJDIR)big_bss
	rm big_bss.failed

hello32.test: hello32.failed hello32.tested
	# test 32 bit app with 64 bit pin, this should fail
	-$(PIN) -- ./hello32 >& hello32.out
	# and we expect to see an error message
	grep 'expected binary for the ' hello32.out
	-$(PIN) -error_file $<.xml  -logfile $<.log -- ./hello32 >& hello32.out
	grep 'expected binary for the ' $<.xml
	rm hello32.failed $<.xml $<.log

statica_locktest.test: $(OBJDIR)statica_locktest$(SATOOL_SUFFIX) statica_locktest.tested statica_locktest.failed
	./$(OBJDIR)statica_locktest$(SATOOL_SUFFIX) -i $(OBJDIR)statica_locktest
	rm statica_locktest.failed

win_syscallenum.test: $(OBJDIR)win_syscallenum$(PINTOOL_SUFFIX) $(TESTAPP) win_syscallenum.tested win_syscallenum.failed
	$(PIN) -t $< -- $(TESTAPP)
	rm win_syscallenum.failed

# On Linux, virual segments mode is default. In order to check the "segments effective address" mode,
# vurtual segments should be switched off
segmented_ea_verifier.test : $(OBJDIR)segmented_ea_verifier$(PINTOOL_SUFFIX) $(OBJDIR)thread_segmented_ea segmented_ea_verifier.tested segmented_ea_verifier.failed
	$(PIN) -xyzzy -virtual_segments 0  -t $< -- ./$(OBJDIR)thread_segmented_ea >  $<.out 2>&1
	grep  "SUCCESS" $<.out
	rm $<.out
	$(PIN) -xyzzy -virtual_segments 1  -t $< -- ./$(OBJDIR)thread_segmented_ea >  $<.out 2>&1
	grep  "SUCCESS" $<.out
	rm segmented_ea_verifier.failed $<.out

ea_verifier_addr16.test : $(OBJDIR)ea_verifier_addr16$(PINTOOL_SUFFIX) $(OBJDIR)thread_ea_addr16 ea_verifier_addr16.tested ea_verifier_addr16.failed
	$(PIN) -xyzzy -virtual_segments 0 -t $< -- ./$(OBJDIR)thread_ea_addr16 >  $<.out 2>&1
	grep  "SUCCESS" $<.out 
	rm $<.out
	$(PIN) -xyzzy -virtual_segments 1 -t $< -- ./$(OBJDIR)thread_ea_addr16 >  $<.out 2>&1
	grep  "SUCCESS" $<.out 
	rm ea_verifier_addr16.failed $<.out

seg_override_win1.test : $(OBJDIR)segmented_ea_verifier$(PINTOOL_SUFFIX) $(OBJDIR)seg_override_app1 seg_override_win1.tested seg_override_win1.failed
	$(PIN)  -t $< -- ./$(OBJDIR)seg_override_app1 >  $<.out 2>&1
	grep  "SUCCESS" $<.out 
	rm seg_override_win1.failed $<.out

# Test reuse of memory pages
memory_reuse.test : memory_reuse.tested memory_reuse.failed
	touch memory_reuse.makefile.copy; rm memory_reuse.makefile.copy
	$(PIN) -xyzzy -malloc_reuse_pages -- $(TESTAPP) makefile memory_reuse.makefile.copy
	$(PIN_DIFF) makefile memory_reuse.makefile.copy
	rm memory_reuse.failed memory_reuse.makefile.copy

# Tests delivery of dll load / unload events for dynamically loaded images in JIT mode
load_dummy_dll.test: %.test : $(OBJDIR)load_dummy_dll_tool$(PINTOOL_SUFFIX) $(OBJDIR)load_dummy_dll_app %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)load_dummy_dll$(EXEEXT)
	grep -c -i "dummy_dll.dll loaded" imageload.out | grep "2"
	grep -c -i "dummy_dll.dll unloaded" imageload.out | grep "2"
	rm load_dummy_dll.failed imageload.out

# Tests delivery of dll load / unload events for dynamically loaded images in probe mode
load_dummy_dll_probe.test: $(OBJDIR)load_dummy_dll_tool$(PINTOOL_SUFFIX) $(OBJDIR)load_dummy_dll_app load_dummy_dll_probe.tested load_dummy_dll_probe.failed
	$(PIN) -probe -t $< -- $(OBJDIR)load_dummy_dll$(EXEEXT)
	grep -c -i "dummy_dll.dll loaded" imageload.out | grep "2"
	grep -c -i "dummy_dll.dll unloaded" imageload.out | grep "2"
	rm load_dummy_dll_probe.failed imageload.out

# Tests dbghelp.dll version checking
dbghelp_version.test: $(OBJDIR)load_dummy_dll_tool$(PINTOOL_SUFFIX) $(OBJDIR)load_dummy_dll_app dbghelp_version.tested dbghelp_version.failed
	mkdir -p pintempdir
	# Copy pin binaries and dbghelp.dll to temp directory, run pin from the directory
	cp -r $(dir $(PIN_EXE))/*.exe pintempdir
	cp -r $(dir $(PIN_EXE))/*.dll pintempdir
	# First run with proper dbghelp.dll
	pintempdir/pin -probe -xyzzy -log_server 1 -logfile dbghelp.log -t $< -xyzzy -log_server 1 -logfile dbghelp.log -- $(OBJDIR)load_dummy_dll$(EXEEXT)
	grep -c -i "dbghelp.dll version mismatch" dbghelp.log* | grep "0"
	# Second run with unsupported system dbghelp.dll
	rm -f pintempdir/dbghelp.dll dbghelp.log* imageload.out
	pintempdir/pin -probe -xyzzy -log_server 1 -logfile dbghelp.log -t $< -xyzzy -log_server 1 -logfile dbghelp.log -- $(OBJDIR)load_dummy_dll$(EXEEXT)
	grep -i "dbghelp.dll version mismatch" dbghelp.log*
	rm -rf pintempdir dbghelp.log* dbghelp_version.failed imageload.out

# Tests proper processing of ia32 dll loaded in Intel 64 process on Windows
x86dll.test: %.test : $(OBJDIR)load_dummy_dll_tool$(PINTOOL_SUFFIX) $(OBJDIR)x86dll_app$(EXEEXT) %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)x86dll_app$(EXEEXT)
	rm x86dll.failed imageload.out

# Tests proper delivery of symbols for rebased dll in JIT mode
rebase_dll.test: %.test : $(OBJDIR)rebase_dll_tool$(PINTOOL_SUFFIX) $(OBJDIR)rebase_dll_app %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)rebase_dll$(EXEEXT)
	grep -c -i "nothing" imageload.out | grep "2"
	rm rebase_dll.failed imageload.out

# Tests proper delivery of symbols for rebased dll in probe mode
rebase_dll_probe.test: %.test : $(OBJDIR)rebase_dll_tool$(PINTOOL_SUFFIX) $(OBJDIR)rebase_dll_app %.tested %.failed
	$(PIN) -probe -t $< -- $(OBJDIR)rebase_dll$(EXEEXT)
	grep -c -i "nothing" imageload.out | grep "2"
	rm rebase_dll_probe.failed imageload.out

# Tests IMG_Open on Windows
img_open_win.test: %.test : $(OBJDIR)img_open_win$(PINTOOL_SUFFIX) $(OBJDIR)rebase_dll_app %.tested %.failed
	$(PIN) -probe -t $< -- $(OBJDIR)rebase_dll$(EXEEXT)
	grep -c -i "nothing" imageload.out | grep "3"
	rm img_open_win.failed imageload.out

# Tests setting probe if base relocation is in probed bytes
baserel_in_probe.test: %.test : $(OBJDIR)baserel_in_probe_tool$(PINTOOL_SUFFIX) $(OBJDIR)rebase_dll2_app %.tested %.failed
	$(PIN) -probe -t $< -- $(OBJDIR)rebase_dll2$(EXEEXT) > baserel_in_probe.out
	grep -c -i "fixup is handled properly" baserel_in_probe.out | grep "4"
	rm baserel_in_probe.failed baserel_in_probe.out

# Tests PIN_InitSymbolsAlt(EXPORT_SYMBOLS) API and exports only support without external symbol server
exports_only.test: %.test : $(OBJDIR)exports_only_tool$(PINTOOL_SUFFIX) $(OBJDIR)rebase_dll_app %.tested %.failed
	$(PIN_EXE) $(PIN_TEST_FLAGS) -xyzzy -log_server 1 -logfile pinsm.log -t $< -xyzzy -log_server 1 -logfile pinsm.log -- $(OBJDIR)rebase_dll$(EXEEXT)
	grep -c -i "nothing" imageload.out | grep "2"
	test `rm -vf pinsm.log.* | wc -l` -eq "0"
	rm exports_only.failed imageload.out

# Tests PIN_InitSymbolsAlt(EXPORT_SYMBOLS) API and exports only support without external symbol server in probe mode
exports_only_probe.test: %.test : $(OBJDIR)exports_only_tool$(PINTOOL_SUFFIX) $(OBJDIR)rebase_dll_app %.tested %.failed
	$(PIN_EXE) $(PIN_TEST_FLAGS) -probe -xyzzy -log_server 1 -logfile pinsmp.log -t $< -xyzzy -log_server 1 -logfile pinsmp.log -- $(OBJDIR)rebase_dll$(EXEEXT)
	grep -c -i "nothing" imageload.out | grep "2"
	test `rm -vf pinsmp.log.* | wc -l` -eq "0"
	rm exports_only_probe.failed imageload.out

# Tests correct retrieval of section name from PE section header
# Ensure section name does not contain trailing null symbols
secname.test: %.test : $(OBJDIR)secname_tool$(PINTOOL_SUFFIX) $(OBJDIR)secname_app$(EXEEXT) %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)secname_app$(EXEEXT)
	grep -q " dsec found" secname.out
	grep -q " asection found" secname.out
	rm secname.failed secname.out


# The test pause_tool is used to test "pause_tool".
# Therefore we need to be built it with debug info
$(OBJDIR)pause_tool.o: pause_tool.cpp
	$(CXX) -g $(COPT) $(CXXFLAGS) $(PIN_CXXFLAGS) $(OUTOPT)$@ $<
 
$(OBJDIR)pause_tool$(PINTOOL_SUFFIX): $(OBJDIR)pause_tool.o
	$(CXX) $(PIN_LDFLAGS) -g ${LINK_OUT}$@ $< $(PIN_LIBS)

pause_tool.test: $(OBJDIR)pause_tool$(PINTOOL_SUFFIX) $(OBJDIR)simple pause_tool.tested pause_tool.failed
	rm -f $(OBJDIR)$(@:.test=.out)
	$(PIN) -pause_tool 20 -t $(OBJDIR)pause_tool$(PINTOOL_SUFFIX) -- $(OBJDIR)simple > $(OBJDIR)$(@:.test=.out) &
	until grep 'add-symbol-file' $(OBJDIR)$(@:.test=.out) > /dev/null; \
	    do sleep 1; done
	sleep 1
	awk '''NR==1 { print "attach ", $$NF }''' < $(OBJDIR)$(@:.test=.out) > $(OBJDIR)$(@:.test=.gdbin)
	grep 'add-symbol-file' $(OBJDIR)$(@:.test=.out) >> $(OBJDIR)$(@:.test=.gdbin)
	cat $(@:.test=.gdb) >> $(OBJDIR)$(@:.test=.gdbin)
	$(GDB) -batch -x $(OBJDIR)$(@:.test=.gdbin) -n $(PINBIN) > $(OBJDIR)$(@:.test=.gdbout) 2>&1
	tail -5 $(OBJDIR)$(@:.test=.gdbout) > $(OBJDIR)$(@:.test=.res)
	diff pause_tool.ref $(OBJDIR)$(@:.test=.res)
	rm -f pause_tool.failed

elf_symbols.test: %.test: $(OBJDIR)elf_symbols$(PINTOOL_SUFFIX) %.tested %.failed
	$(PIN) -t $< -o $*.out -- $(TESTAPP) makefile $<.makefile.copy 
	grep "passed successfully" $*.out
	rm -f $*.failed $*.out $<.makefile.copy


## build rules

$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)%.o : %.cc
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(STATIC_TOOLS): $(PIN_LIBNAMES)

$(OBJDIR)%.o : %.S
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)%.o : %.s
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(STATIC_TOOLS): %$(SATOOL_SUFFIX): %.o
	${PIN_LD} $(PIN_SALDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(SAPIN_LIBS) $(DBG)

$(OBJDIR)mt_tool$(PINTOOL_SUFFIX) :  $(OBJDIR)mt_tool.o $(SYS_LIB_TOOL)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< $(SYS_LIB_TOOL) ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)win_multi_dll_pintool : $(OBJDIR)main_dll$(PINTOOL_SUFFIX) $(OBJDIR)static_secondary_dll$(PINTOOL_SUFFIX) $(OBJDIR)dynamic_secondary_dll$(PINTOOL_SUFFIX) 

$(OBJDIR)main_dll$(PINTOOL_SUFFIX) : $(OBJDIR)main_dll.o $(OBJDIR)static_secondary_dll$(PINTOOL_SUFFIX)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) $(OBJDIR)static_secondary_dll.lib

$(OBJDIR)align_access$(PINTOOL_SUFFIX) : $(OBJDIR)align_access.o 
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)static_secondary_dll$(PINTOOL_SUFFIX) : $(OBJDIR)static_secondary_dll.o 
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)dynamic_secondary_dll$(PINTOOL_SUFFIX) : $(OBJDIR)dynamic_secondary_dll.o 
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)      

$(OBJDIR)sse-ref$(PINTOOL_SUFFIX) : $(OBJDIR)sse-ref.o $(SSE-REF_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) $(SSE-REF_ASM_OBJ)

$(OBJDIR)flag_spill_fill_tool1$(PINTOOL_SUFFIX): $(OBJDIR)flag_spill_fill_tool1.o $(OBJDIR)$(FLAG_SPILL_FILL_TOOL1_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) $(OBJDIR)$(FLAG_SPILL_FILL_TOOL1_ASM_OBJ)	

$(OBJDIR)analysis_flag_overwrite_tool1$(PINTOOL_SUFFIX): $(OBJDIR)analysis_flag_overwrite_tool1.o $(OBJDIR)$(ANALYSIS_FLAG_OVERWRITE_TOOL1_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)analysis_flag_overwrite_tool2$(PINTOOL_SUFFIX): $(OBJDIR)analysis_flag_overwrite_tool2.o $(OBJDIR)$(ANALYSIS_FLAG_OVERWRITE_TOOL2_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 	

$(OBJDIR)analysis_flag_overwrite_tool3$(PINTOOL_SUFFIX): $(OBJDIR)analysis_flag_overwrite_tool3.o $(OBJDIR)$(ANALYSIS_FLAG_OVERWRITE_TOOL3_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)df_test_tool1$(PINTOOL_SUFFIX): $(OBJDIR)df_test_tool1.o $(OBJDIR)$(DF_TEST_TOOL1_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)inline_opt_test$(PINTOOL_SUFFIX): $(OBJDIR)inline_opt_test.o $(OBJDIR)$(INLINE_OPT_TEST_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)inline_opt_test_df1$(PINTOOL_SUFFIX): $(OBJDIR)inline_opt_test_df1.o $(OBJDIR)$(INLINE_OPT_TEST_DF_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)inline_opt_test_df2$(PINTOOL_SUFFIX): $(OBJDIR)inline_opt_test_df2.o $(OBJDIR)$(INLINE_OPT_TEST_DF_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)inline_opt_test_df3$(PINTOOL_SUFFIX): $(OBJDIR)inline_opt_test_df3.o $(OBJDIR)$(INLINE_OPT_TEST_DF_APP_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

#$(OBJDIR)flag_ac_tool1$(PINTOOL_SUFFIX): $(OBJDIR)flag_ac_tool.o 
#	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

#$(OBJDIR)flag_ac_tool2$(PINTOOL_SUFFIX): $(OBJDIR)flag_ac_tool.o 
#	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

#$(OBJDIR)flag_ac_tool3$(PINTOOL_SUFFIX): $(OBJDIR)flag_ac_tool.o 
#	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)flag_ac_win_tool$(PINTOOL_SUFFIX): $(OBJDIR)flag_ac_win_tool.o $(OBJDIR)$(FLAG_AC_APP_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)	

$(OBJDIR)reg_operands_test_tool$(PINTOOL_SUFFIX): $(OBJDIR)reg_operands_test_tool.o 
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)df_test_tool2$(PINTOOL_SUFFIX): $(OBJDIR)df_test_tool2.o $(OBJDIR)$(DF_TEST_TOOL2_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 	

$(OBJDIR)df_test_tool3$(PINTOOL_SUFFIX): $(OBJDIR)df_test_tool3.o $(OBJDIR)$(DF_TEST_TOOL3_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 	

$(OBJDIR)df_test_tool4$(PINTOOL_SUFFIX): $(OBJDIR)df_test_tool4.o $(OBJDIR)$(DF_TEST_TOOL4_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 	

$(OBJDIR)df_test_tool5$(PINTOOL_SUFFIX): $(OBJDIR)df_test_tool5.o $(OBJDIR)$(DF_TEST_TOOL5_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)move_then_test$(PINTOOL_SUFFIX): $(OBJDIR)move_then_test.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)inlined-stack-arg2$(PINTOOL_SUFFIX): $(OBJDIR)inlined-stack-arg2.o $(OBJDIR)$(INLINED_STACK_ARG2_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)safecopy_mxcsr$(PINTOOL_SUFFIX):  $(OBJDIR)$(MXCSR_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)context_state_apis$(PINTOOL_SUFFIX):  $(OBJDIR)$(SUPPORTS_AVX_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

$(OBJDIR)jcx_addr16_tool$(PINTOOL_SUFFIX): $(OBJDIR)jcx_addr16_tool.o 
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 	

$(OBJDIR)load_dummy_dll_tool$(PINTOOL_SUFFIX) : load_dummy_dll_tool.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $< /link /manifest:no $(PIN_LDFLAGS) $(PIN_LIBS) ${LINK_OUT}$@

$(OBJDIR)rebase_dll_tool$(PINTOOL_SUFFIX) : rebase_dll_tool.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $< /link /manifest:no $(PIN_LDFLAGS) $(PIN_LIBS) ${LINK_OUT}$@

$(OBJDIR)img_open_win$(PINTOOL_SUFFIX) : img_open_win.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $< /link /manifest:no $(PIN_LDFLAGS) $(PIN_LIBS) ${LINK_OUT}$@

$(OBJDIR)baserel_in_probe_tool$(PINTOOL_SUFFIX) : baserel_in_probe_tool.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $< /link /manifest:no $(PIN_LDFLAGS) $(PIN_LIBS) ${LINK_OUT}$@

$(OBJDIR)exports_only_tool$(PINTOOL_SUFFIX) : exports_only_tool.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $< /link /manifest:no $(PIN_LDFLAGS) $(PIN_LIBS) ${LINK_OUT}$@

$(OBJDIR)secname_tool$(PINTOOL_SUFFIX) : secname_tool.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $< /link /manifest:no $(PIN_LDFLAGS) $(PIN_LIBS) ${LINK_OUT}$@

$(OBJDIR)stackunswitch: $(OBJDIR)stackunswitch.o $(OBJDIR)unswitcher.o
	$(CXX) $(PIN_LDFLAGS) ${OUTOPT}stackunswitch $(OBJDIR)stackunswitch.o $(OBJDIR)unswitcher.o $(PIN_LIBS)

$(OBJDIR)xmmtest$(PINTOOL_SUFFIX):  $(SET_XMM_SCRATCH_REGS_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $+ ${PIN_LPATHS} $(PIN_LIBS) $(DBG) 

ifneq ($(TARGET_OS),b)
    DL_LIB = -ldl
else
    DL_LIB =
endif

$(OBJDIR)dlopen: $(OBJDIR)dlopen.o $(OBJDIR)dummy.so
	$(CXX) $(PIN_LDFLAGS) ${OUTOPT}$@ $< $(PIN_LIBS) $(DL_LIB} $(DBG)

$(OBJDIR)dltest: dltest.c one.c two.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)two.so -fpic -shared two.c -g
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)one.so -fpic -shared one.c -g
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)dltest $(APP_CXXFLAGS) dltest.c $(DL_LIB) -Wl,-rpath,`pwd`\/$(OBJDIR) -g

$(OBJDIR)dltest2: dltest2.c $(OBJDIR)dltest
	$(CC) ${OUTOPT}$@ $(APP_CXXFLAGS) dltest2.c $(DL_LIB) -Wl,-rpath,`pwd`\/$(OBJDIR) -g

$(OBJDIR)dltest-mac: dltest.c one.c two.c
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)two.so -fPIC -dynamiclib two.c -g
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)one.so -fPIC -dynamiclib one.c -g
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)dltest-mac $(APP_CXXFLAGS) dltest.c $(DL_LIB) -g

$(OBJDIR)oneprog: oneprog.c
	$(CC) ${OUTEXE}$(OBJDIR)oneprog $(NO_OPTIMIZE) $(APP_CXXFLAGS) oneprog.c

$(OBJDIR)redblue: redblue.s
	$(CC) $(APP_CXXFLAGS) -o $@ -g redblue.s

special.test: $(OBJDIR)special$(PINTOOL_SUFFIX) special.tested special.failed $(OBJDIR)redblue
	$(PIN) -t $< -- ./$(OBJDIR)redblue
	$(PIN_CMP) special.out special.ref
	rm special.failed

pusha_popa.test: $(OBJDIR)pusha_popa$(EXEEXT) pusha_popa.tested pusha_popa.failed
	$(PIN) -- ./$(OBJDIR)pusha_popa$(EXEEXT)
	rm pusha_popa.failed

$(OBJDIR)pusha_popa$(EXEEXT): $(OBJDIR)pusha_popa.$(OBJEXT) $(OBJDIR)pusha_popa_asm.$(OBJEXT)
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS)  $(OUTEXE)$@ $+ $(APP_CXXLINK_FLAGS)

$(OBJDIR)pusha_popa.$(OBJEXT): pusha_popa.cpp
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS)  $(COPT) $(OUTOPT)$@ $<

$(OBJDIR)pusha_popa_asm.obj: pusha_popa_asm.asm
	ml /nologo /c /Fo$@ $<

$(OBJDIR)pusha_popa_asm.o: pusha_popa_asm.s
	$(CC) $(APP_CXXFLAGS) -c $< -o $@

$(OBJDIR)load_dummy_dll_app : $(OBJDIR)load_dummy_dll$(EXEEXT) $(OBJDIR)dummy_dll$(PINTOOL_SUFFIX)

$(OBJDIR)load_dummy_dll$(EXEEXT): load_dummy_dll.c
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)dummy_dll$(PINTOOL_SUFFIX): dummy_dll.c
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS) $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS) /dll /noentry /BASE:0x10000000

$(OBJDIR)x86dll_app$(EXEEXT): x86dll_app.cpp
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS)
	touch x86dll.def
	${LINKER} /DLL /MACHINE:X86 /DEF:x86dll.def $(PIN_EXTRA_LDFLAGS) /NOENTRY /MANIFEST:NO ${LINK_OUT}$(OBJDIR)x86dll.dll

$(OBJDIR)rebase_dll_app : $(OBJDIR)rebase_dll$(EXEEXT) $(OBJDIR)dummy_dll$(PINTOOL_SUFFIX) $(OBJDIR)dummy_dll2$(PINTOOL_SUFFIX)

$(OBJDIR)rebase_dll2_app : $(OBJDIR)rebase_dll2$(EXEEXT) $(OBJDIR)dummy_dll$(PINTOOL_SUFFIX) $(OBJDIR)dummy_dll2$(PINTOOL_SUFFIX)

$(OBJDIR)rebase_dll$(EXEEXT): rebase_dll.c
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)rebase_dll2$(EXEEXT): rebase_dll.c dummy_dll.c
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS)  $(OUTEXE)$@ $^ $(APP_CXXLINK_FLAGS) /FIXED:NO /BASE:0

$(OBJDIR)dummy_dll2$(PINTOOL_SUFFIX) : $(OBJDIR)dummy_dll$(PINTOOL_SUFFIX)
	cp -f $< $@

$(OBJDIR)secname_app$(EXEEXT): secname_app.cpp
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS)  $(OUTEXE)$@ $< $(APP_CXXLINK_FLAGS)

$(OBJDIR)inlined-stack-arg-noopt.o: inlined-stack-arg.cpp
	$(CXX) ${COPT}  $(CXXFLAGS) $(PIN_CXXFLAGS_NOOPT) ${OUTOPT}$@ $<

$(OBJDIR)inlined-stack-arg-noopt$(PINTOOL_SUFFIX): $(OBJDIR)inlined-stack-arg-noopt.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)inlined-stack-arg1-noopt.o: inlined-stack-arg1.cpp
	$(CXX) ${COPT}  $(CXXFLAGS)  $(PIN_CXXFLAGS_NOOPT)  ${OUTOPT}$@ $<

$(OBJDIR)inlined-stack-arg1-noopt$(PINTOOL_SUFFIX): $(OBJDIR)inlined-stack-arg1-noopt.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)exception_context_monitor$(PINTOOL_SUFFIX): $(OBJDIR)exception_context_monitor.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)pushtest$(EXEEXT): pushtest.c pushw.s
	$(CXX) $(CXXFLAGS) $(APP_CXXFLAGS)  $(OUTEXE)$@ $^ $(APP_CXXLINK_FLAGS)

jcx_addr16_tool.test: $(OBJDIR)jcx_addr16_tool$(PINTOOL_SUFFIX) $(OBJDIR)jcx_addr16_app jcx_addr16_tool.tested jcx_addr16_tool.failed
	$(PIN)  -xyzzy -late_injection 1 -t $< -- ./$(OBJDIR)jcx_addr16_app  >  $<.out 2>&1
	$(PIN_DIFF) $<.out jcx_addr16_tool.reference
	rm jcx_addr16_tool.failed $<.out

inlined-stack-arg.test: $(OBJDIR)inlined-stack-arg$(PINTOOL_SUFFIX) inlined-stack-arg.tested inlined-stack-arg.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline 1 $(OPT_STACK_PARAM_FAILURE_ASSERT) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	$(PIN_DIFF) inlined-stack-arg.out inlined-stack-arg.reference
	rm inlined-stack-arg.failed $<.makefile.copy inlined-stack-arg.out

inlined-stack-arg-noopt.test: $(OBJDIR)inlined-stack-arg-noopt$(PINTOOL_SUFFIX) inlined-stack-arg-noopt.tested inlined-stack-arg-noopt.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline 1 -t $< -o inlined-stack-arg-noopt.out -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	$(PIN_DIFF) inlined-stack-arg-noopt.out inlined-stack-arg.reference
	rm inlined-stack-arg-noopt.failed $<.makefile.copy inlined-stack-arg-noopt.out

inlined-stack-arg1.test: $(OBJDIR)inlined-stack-arg1$(PINTOOL_SUFFIX) inlined-stack-arg1.tested inlined-stack-arg1.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline 1 $(OPT_STACK_PARAM_FAILURE_ASSERT) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	$(PIN_DIFF) inlined-stack-arg1.out inlined-stack-arg1.reference
	$(PIN) -log_inline -logfile $<.log  -xyzzy -inline 1 $(OPT_STACK_PARAM_FAILURE_ASSERT) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	grep -q IARG_CONTEXT $<.log
	rm inlined-stack-arg1.failed $<.makefile.copy inlined-stack-arg1.out $<.log

inlined-stack-arg1-noopt.test: $(OBJDIR)inlined-stack-arg1-noopt$(PINTOOL_SUFFIX) inlined-stack-arg1-noopt.tested inlined-stack-arg1-noopt.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline 1 -region-max-bbls 1 -t $< -o inlined-stack-arg1-noopt.out -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	$(PIN_DIFF) inlined-stack-arg1-noopt.out inlined-stack-arg1.reference
	rm inlined-stack-arg1-noopt.failed $<.makefile.copy inlined-stack-arg1-noopt.out

inlined-stack-arg2.test: $(OBJDIR)inlined-stack-arg2$(PINTOOL_SUFFIX) inlined-stack-arg2.tested inlined-stack-arg2.failed
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN) -xyzzy -inline 1 $(OPT_STACK_PARAM_FAILURE_ASSERT) -t $< -- $(TESTAPP) makefile $<.makefile.copy
	$(PIN_CMP) makefile $<.makefile.copy
	$(PIN_DIFF) inlined-stack-arg2.out inlined-stack-arg2.reference
	rm inlined-stack-arg2.failed $<.makefile.copy inlined-stack-arg2.out

earlymalloc.test: $(OBJDIR)earlymalloc$(PINTOOL_SUFFIX) earlymalloc.tested earlymalloc.failed
	$(PIN) -t $< -- $(TESTAPP) makefile earlymalloc.makefile.copy
	rm earlymalloc.failed earlymalloc.makefile.copy


ifeq ($(SOTOOL),1)
$(OBJDIR)earlymallocexe:
	$(MAKE) SOTOOL=0 $@

earlymallocexe.test: $(OBJDIR)earlymallocexe earlymallocexe.tested earlymallocexe.failed
	$(PIN) -t $< -- $(TESTAPP) makefile earlymallocexe.makefile.copy
	rm earlymallocexe.failed earlymallocexe.makefile.copy
else
$(OBJDIR)earlymallocexe.$(OBJEXT): earlymalloc.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(OBJDIR)earlymallocexe$(PINTOOL_SUFFIX): $(OBJDIR)earlymallocexe.$(OBJEXT)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

earlymallocexe.test: $(OBJDIR)earlymallocexe$(PINTOOL_SUFFIX) earlymallocexe.tested earlymallocexe.failed
	$(PIN) -t $< -- $(TESTAPP) makefile earlymallocexe.makefile.copy
	rm earlymallocexe.failed earlymallocexe.makefile.copy
endif

emu_stack.test: $(OBJDIR)emu_stack$(PINTOOL_SUFFIX) emu_stack.tested emu_stack.failed
	$(PIN) -t $< -- $(TESTAPP) makefile emu_stack.makefile.copy
	rm emu_stack.failed emu_stack.makefile.copy

emu_jumps.test: $(OBJDIR)emu_jumps$(PINTOOL_SUFFIX) emu_jumps.tested emu_jumps.failed
	$(PIN) -t $< -- $(TESTAPP) makefile emu_jumps.makefile.copy
	rm emu_jumps.failed emu_jumps.makefile.copy

$(OBJDIR)swizzle_seg_app_l: swizzle_seg_app.cpp swizzle_seg_app_lin_$(TARGET).s
	$(CXX) $(APP_CXXFLAGS) $^ -o $@

$(OBJDIR)swizzle_seg_app_b: swizzle_seg_app.cpp swizzle_seg_app_lin_$(TARGET).s
	$(CXX) $(APP_CXXFLAGS) $^ -o $@

$(OBJDIR)swizzle_seg_app_w$(EXEEXT): swizzle_seg_app.cpp swizzle_seg_app.def swizzle_seg_app_win_$(TARGET).asm
	$(MASM) /c /Fo$(OBJDIR)swizzle_seg_app_win_$(TARGET).obj swizzle_seg_app_win_$(TARGET).asm
	$(CXX) /c $(APP_CXXFLAGS) /Fo$(OBJDIR)swizzle_seg_app.obj swizzle_seg_app.cpp 
	${LINKER} ${LINK_OUT}$@ /DEF:swizzle_seg_app.def $(OBJDIR)swizzle_seg_app.obj $(OBJDIR)swizzle_seg_app_win_$(TARGET).obj

swizzle_seg.test: %.test: $(OBJDIR)swizzle_seg$(PINTOOL_SUFFIX) $(OBJDIR)swizzle_seg_app_$(TARGET_OS)$(EXEEXT) %.tested %.failed
	$(PIN) -t $< -- $(OBJDIR)swizzle_seg_app_$(TARGET_OS)$(EXEEXT) > $*.out
	$(PIN_DIFF)  $*.out $*.reference
	rm $*.failed $*.out

far_ret.test: %.test: $(OBJDIR)far %.tested %.failed
	$< >$*.ref
	$(PIN) -- $< > $*.out
	$(PIN_DIFF)  $*.out $*.ref
	rm $*.failed $*.out $*.ref

tool_malloc.test: %.test: $(OBJDIR)tool_malloc$(PINTOOL_SUFFIX) %.tested %.failed
	rm -f $*.out
	$(PIN) -t $< -o $*.out -xyzzy -statistic -- $(TESTAPP) makefile tool_malloc.makefile.copy
	./check_tool_malloc.sh $*.out pintool.log
	rm $*.failed $*.out pintool.log tool_malloc.makefile.copy 

#The loader test doesn't run cross-architecture and on 2.4 kernels
#It also can't run on machines with apparmor protection for ldd and ld-linux.so
loader.test: %.test: $(OBJDIR)fini$(PINTOOL_SUFFIX) %.tested %.failed
	if ! test -f /etc/apparmor.d/usr.bin.ldd; then \
	$(PIN) -follow_execv -t $< -- ldd $(TESTAPP); fi
	rm $*.failed

$(OBJDIR)far: farmain.cpp far.s
	$(CXX) $(APP_CXXFLAGS) $^  -o $@


# must manually enable sse2 for older gcc verions used in sanity
$(OBJDIR)intrin.o : intrin.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(SSE2) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

intrin.test: $(OBJDIR)intrin$(PINTOOL_SUFFIX) 
	touch $<.makefile.copy; rm $<.makefile.copy
	$(PIN)   -t $< -- $(TESTAPP) makefile $<.makefile.copy


secure_scl.test:  secure_scl.tested secure_scl.failed
	echo $(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) /D_SECURE_SCL=1 secure_scl_test.cpp > compile_secure_scl.bat
	echo "dir compile_secure_scl.bat" >> compile_secure_scl.bat
	chmod +x ./compile_secure_scl.bat
	./compile_secure_scl.bat > secure_scl_test.out
	grep "PIN is compiled with /D_SECURE_SCL=0 so tool must also be compiled with /D_SECURE_SCL=0" secure_scl_test.out
	rm secure_scl.failed secure_scl_test.out

check_legal_callbacks.test: %.test: $(OBJDIR)check_legal_callbacks$(PINTOOL_SUFFIX) %.tested %.failed
	-$(PIN) -t $< -logfile check_legal_callbacks.out -- $(TESTAPP) makefile $<.makefile.copy
	grep "Probe mode callback can be used only with PIN_StartProgramProbed" check_legal_callbacks.out
	rm -f check_legal_callbacks.out
	-$(PIN) -probe -t $< -logfile check_legal_callbacks.out -- $(TESTAPP) makefile $<.makefile.copy
	grep "JIT mode callback can be used only with PIN_StartProgram" check_legal_callbacks.out
	rm $*.failed check_legal_callbacks.out
    
undecorate.test: %.test: $(OBJDIR)undecorate$(PINTOOL_SUFFIX) $(OBJDIR)undecorate_app %.tested %.failed
	$(PIN) -t $< -o $(OBJDIR)$*.out  -- $(OBJDIR)undecorate_app
	grep method $(OBJDIR)$*.out
	rm $*.failed $(OBJDIR)$*.out
    
$(OBJDIR)undecorate_app: undecorate_app.cpp 
	$(CXX) $(APP_CXXFLAGS) $^ -g -o $@


## overriding some build rules
$(OBJDIR)internal_exception_handler_lin$(PINTOOL_SUFFIX): $(OBJDIR)internal_exception_handler_lin.o exception_tool_$(TARGET).s
	$(CC) $(APP_CXXFLAGS) -c exception_tool_$(TARGET).s -o $(OBJDIR)exception_tool_$(TARGET).o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG) $(OBJDIR)exception_tool_$(TARGET).o

knob_writeonce.test : $(OBJDIR)knob_writeonce$(PINTOOL_SUFFIX) knob_writeonce.tested knob_writeonce.failed $(OBJDIR)$(HELLO)
	@echo "This 'knob_writeonce' run should not fail"
	$(PIN) -t $< -test value1 -test value1 -- ./$(OBJDIR)$(HELLO) > $(OBJDIR)$(@:.test=1.out)
	grep -c "trying to overwrite write-once knob" $(OBJDIR)$(@:.test=1.out) | grep 0
	@echo "This 'knob_writeonce' run is expected to fail and to dump core file"
	-$(PIN) -t $< -test value1 -test value2 -- ./$(OBJDIR)$(HELLO) > $(OBJDIR)$(@:.test=2.out)
	grep "trying to overwrite write-once knob" $(OBJDIR)$(@:.test=2.out)
	rm knob_writeonce.failed

knob_duplicate_name.test : $(OBJDIR)knob_duplicate_name$(PINTOOL_SUFFIX) knob_duplicate_name.tested knob_duplicate_name.failed $(OBJDIR)$(HELLO)
	@echo "This 'knob_duplicate_name' run is expected to fail and to dump core file"
	-$(PIN) -t $< -- ./$(OBJDIR)$(HELLO) > $(OBJDIR)$(@:.test=.out)
	grep "duplicate option" $(OBJDIR)$(@:.test=.out) | grep "CheckAllKnobs"
	rm knob_duplicate_name.failed


knob_help.test : knob_help.tested knob_help.failed
	$(PIN) -help > $(OBJDIR)$(@:.test=.out)
	grep -c "General switches" $(OBJDIR)$(@:.test=.out) | grep -v "0"
	grep -c "E: Missing application name" $(OBJDIR)$(@:.test=.out) | grep "0"
	rm knob_help.failed


knob_version.test : knob_version.tested knob_version.failed
	$(PIN) -version > $(OBJDIR)$(@:.test=.out)
	grep -c "version.cpp" $(OBJDIR)$(@:.test=.out) | grep -v "0"
	grep -c "Usage" $(OBJDIR)$(@:.test=.out) | grep "0"
	grep -c "E: Missing application name" $(OBJDIR)$(@:.test=.out) | grep "0"
	rm knob_version.failed


knob_version_long_lin.test : knob_version_long_lin.tested knob_version_long_lin.failed
	$(PIN) -version_long > $(OBJDIR)$(@:.test=.out)
	grep -c "version.cpp" $(OBJDIR)$(@:.test=.out) | grep -v "0"
	grep -c "/proc/version" $(OBJDIR)$(@:.test=.out) | grep -v "0"
	grep -c "/proc/meminfo" $(OBJDIR)$(@:.test=.out) | grep -v "0"
	grep -c "/proc/cpuinfo" $(OBJDIR)$(@:.test=.out) | grep -v "0"
	grep -c "Usage" $(OBJDIR)$(@:.test=.out) | grep "0"
	grep -c "E: Missing application name" $(OBJDIR)$(@:.test=.out) | grep "0"
	rm knob_version_long_lin.failed


funcarg_ep.test: %.test: $(OBJDIR)funcarg_ep$(PINTOOL_SUFFIX) $(OBJDIR)funcarg_ep_app$(EXEEXT) %.failed %.tested
	$(PIN) -t $(OBJDIR)funcarg_ep$(PINTOOL_SUFFIX) -o $*.out -- $(OBJDIR)funcarg_ep_app
	$(PIN_DIFF)  $*.out $*.reference_$(TARGET)
	rm $*.failed $*.out 

$(OBJDIR)funcarg_ep_app$(EXEEXT): funcarg_ep_app.cpp
	$(CXX) $(APP_CXXFLAGS) $< ${OUTEXE}$@ 

fp_args.test: %.test: $(OBJDIR)fp_args$(PINTOOL_SUFFIX) $(OBJDIR)fp_args_app %.failed %.tested
	$(PIN) -t $< -- $(OBJDIR)fp_args_app > $(OBJDIR)$*.out
	$(PIN_DIFF)  $(OBJDIR)$*.out $*.reference
	rm $*.failed $(OBJDIR)$*.out


$(OBJDIR)fp_args_app: fp_args_app.cpp
	$(CXX) $(APP_CXXFLAGS) $^ -g --no-inline -o $@

$(OBJDIR)dlclose_app: dlclose_app.cpp $(OBJDIR)libclose1.so 
	$(CXX) $(APP_CXXFLAGS) dlclose_app.cpp  -o $@ -ldl -g

$(OBJDIR)libclose1.so: one.c $(OBJDIR)libclose2.so
	$(CC) $(APP_CXXFLAGS) ${OUTOPT}$(OBJDIR)libclose1.so -fpic -shared one.c -g -Wl,-lclose2 -Wl,-L$(OBJDIR)

$(OBJDIR)libclose2.so: two.c
	$(CC) $(APP_CXXFLAGS) -o $@ -fpic -shared two.c 

aoti_mutation_tool.test: %.test: $(OBJDIR)aoti_mutation_tool$(PINTOOL_SUFFIX) $(OBJDIR)aoti_mutation_target %.failed %.tested
	$(PIN) -t $< -- $(OBJDIR)aoti_mutation_target > $(OBJDIR)$*.out
	rm $*.failed $(OBJDIR)$*.out

$(OBJDIR)aoti_mutation_target: aoti_mutation_target.c
	$(CC) $(APP_CXXFLAGS) -o $@ $<

#Intel64 application with IA-32 system call
unsup_syscall.test: %.test: $(OBJDIR)unsup_syscall_app %.failed %.tested
	$(PIN) -logfile $*.log -- $(OBJDIR)unsup_syscall_app || grep -q "Pin does not support" $*.log
	rm $*.failed $*.log
	
$(OBJDIR)unsup_syscall_app: unsup_syscall.S
	$(CC) -nostdlib $< -o $@
	
## cleaning
clean:
	-rm -rf $(OBJDIR) *.output  *.out *.tested *.failed *.xml *.makefile.copy 
	-rm -rf pin.log pintool.log ctxtsave.txt memsave.txt hello.c core.* *.core

