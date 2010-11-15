##
## PIN tools
##

##############################################################
#
# Here are some	things you might want to configure
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
	include	../makefile.gnu.config
	STATIC=-static
endif

ifeq ($(TARGET_COMPILER),ms)
	include	../makefile.ms.config
	DBG?=
endif

ifneq ($(ENABLE_VS), 1)
	VS_FLAG	= -xyzzy -virtual_segments 1
else
	VS_FLAG	=
endif

SUF	= $(PINTOOL_SUFFIX)



###============================================================
##
## Set up the test tools
##

TEST_ROOTS = vmovdqu vmovqu_noinline  ymm_ref_actual 
TEST_TOOLS = ymm_ref set_ymm_scratches  ymm_ref_actual 
APPLICATIONS = vmovqu_app ymm_app 

## linux only
## the tests run on ia32 and Intel64 
## Linux 2.4 is not supported 
ifeq ($(TARGET_OS),l)
ifneq ($(TARGET),ipf)

    TEST_TOOLS += attach_detach_jit_tool ymm_ref
    APPLICATIONS += attach_in_sighandler_app detach_in_sighandler_app
    TEST_ROOTS += attach_in_sighandler_ymm attach_in_sighandler_ymm1 detach_in_sighandler_ymm detach_in_sighandler_ymm
    AVX_CHECK_ASM_OBJ = avx_support_$(TARGET).o
    VMOVQU_APP_ASM_OBJ = vmovqu_app_$(TARGET).o
    SET_YMM_SCRATCHES_ASM_OBJ = set_ymm_scratches_$(TARGET).o
    VERIFY_FP_CONTEXT_ASM_OBJ = verify_fp_context_$(TARGET).o
    YMM_APP_ASM_OBJ = ymm_$(TARGET).o
    AVX_CHECK = $(OBJDIR)avx_check
    BUILD_ALL = build_all
    TEST_ALL = test_all
endif
endif


ifeq ($(TARGET_OS),w)
    TEST_TOOLS += exception_context_monitor raise_exception
    APPLICATIONS +=  suspend_context_app_win winapp_exception_ntcontinue
    TEST_ROOTS += exception_context_monitor raise_exception
    AVX_CHECK_ASM_OBJ = avx_support_$(TARGET).obj
    VMOVQU_APP_ASM_OBJ = vmovqu_app_$(TARGET).obj
    SET_YMM_SCRATCHES_ASM_OBJ = set_ymm_scratches_$(TARGET).obj
    VERIFY_FP_CONTEXT_ASM_OBJ = verify_fp_context_$(TARGET).obj
    YMM_APP_ASM_OBJ = ymm_$(TARGET).obj
    AVX_CHECK = $(OBJDIR)avx_check
    BUILD_ALL = build_all
    TEST_ALL = test_all
endif

TEST_ROOTS_NO_AVX_SUPPORT = ymm_ref
TEST_TOOLS_NO_AVX_SUPPORT = ymm_ref


TEST_APPLICATIONS =	$(APPLICATIONS:%=$(OBJDIR)%)
TOOLS =	$(TEST_TOOLS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))
TESTS_TO_RUN = $(TEST_ROOTS:%=%.test)
TESTS_TO_RUN_NO_AVX_SUPPORT = $(TEST_ROOTS_NO_AVX_SUPPORT:%=%.test)

MACHINE_SUPPORTS_AVX_INSTRUCTIONS =`./$(OBJDIR)avx_check|grep -q Yes`
	

test_avx: $(TESTS_TO_RUN)
test_no_avx: $(TESTS_TO_RUN_NO_AVX_SUPPORT)

build_avx: $(TEST_APPLICATIONS) $(TOOLS)
build_no_avx: $(TEST_TOOLS_NO_AVX_SUPPORT:%=$(OBJDIR)%$(PINTOOL_SUFFIX))

$(OBJDIR):
	mkdir -p $(OBJDIR)
	

tests-sanity: test

all: $(BUILD_ALL)

build_all:
	make $(AVX_CHECK)
	if $(MACHINE_SUPPORTS_AVX_INSTRUCTIONS); then \
		echo "Building AVX tools and applications"; \
		make build_avx; \
		else \
		echo "AVX instructions are not supported on this machine"; \
		make build_no_avx; \
	fi

ifeq ($(TARGET_OS),w)	
$(OBJDIR)$(AVX_CHECK_ASM_OBJ): avx_support_$(TARGET).asm
	mkdir -p $(OBJDIR)
	$(MASM) /nologo /c /Fo$@ $<

$(OBJDIR)avx_check: avx_check.c $(OBJDIR)$(AVX_CHECK_ASM_OBJ)
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} avx_check.c  $(OBJDIR)$(AVX_CHECK_ASM_OBJ) -o $@

$(OBJDIR)$(VMOVQU_APP_ASM_OBJ): vmovqu_app_$(TARGET).asm
	mkdir -p $(OBJDIR)
	$(MASM) /nologo /c /Fo$@ $<

$(OBJDIR)$(SET_YMM_SCRATCHES_ASM_OBJ): set_ymm_scratches_$(TARGET).asm
	mkdir -p $(OBJDIR)
	$(MASM) /nologo /c /Fo$@ $<

$(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ): verify_fp_context_$(TARGET).asm
	mkdir -p $(OBJDIR)
	$(MASM) /nologo /c /Fo$@ $<

$(OBJDIR)$(YMM_APP_ASM_OBJ): ymm_$(TARGET).asm
	mkdir -p $(OBJDIR)
	$(MASM) /nologo /c /Fo$@ $<

ifeq ($(TARGET),ia32e)
	ARCHOPT=x64
	arch=64
	ARCHOPT=x64
else
	ARCHOPT=x86
	arch=32
	ARCHOPT=x86
endif
NTDLL_LIB =ntdll_$(TARGET).lib

$(OBJDIR)$(NTDLL_LIB):  ntdll.def
	cl /DWIN$(arch) /D_WIN32_WINNT=0x501 /D_NTDLL_IMPLIB /c /Tcntdll.h /Fo$(OBJDIR)ntdll_$(TARGET).obj /nologo
ifeq ($(TARGET),ia32e)
	lib $(OBJDIR)ntdll_$(TARGET).obj  /machine:x64 /def:ntdll.def /out:$(OBJDIR)$(NTDLL_LIB) /nologo
else
	lib $(OBJDIR)ntdll_$(TARGET).obj  /machine:x86 /def:ntdll.def /out:$(OBJDIR)$(NTDLL_LIB) /nologo
endif

else
$(OBJDIR)$(AVX_CHECK_ASM_OBJ): avx_support_$(TARGET).s
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} -c avx_support_$(TARGET).s -o $(OBJDIR)avx_support_$(TARGET).o

$(OBJDIR)avx_check: avx_check.c $(OBJDIR)$(AVX_CHECK_ASM_OBJ)
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} avx_check.c -Wl,$(OBJDIR)$(AVX_CHECK_ASM_OBJ) -o $@

$(OBJDIR)$(VMOVQU_APP_ASM_OBJ): vmovqu_app_$(TARGET).s
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} -c vmovqu_app_$(TARGET).s -o $(OBJDIR)vmovqu_app_$(TARGET).o

$(OBJDIR)$(SET_YMM_SCRATCHES_ASM_OBJ): set_ymm_scratches_$(TARGET).s
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} -c set_ymm_scratches_$(TARGET).s -o $(OBJDIR)set_ymm_scratches_$(TARGET).o

$(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ): verify_fp_context_$(TARGET).s
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} -c verify_fp_context_$(TARGET).s -o $(OBJDIR)verify_fp_context_$(TARGET).o

$(OBJDIR)$(YMM_APP_ASM_OBJ): ymm_$(TARGET).s
	mkdir -p $(OBJDIR)
	$(CC) ${APP_CXXFLAGS} -c ymm_$(TARGET).s -o $(OBJDIR)ymm_$(TARGET).o

endif


$(OBJDIR)vmovqu_app	: vmovqu_app.c $(OBJDIR)$(VMOVQU_APP_ASM_OBJ)
	$(CC) $(APP_CXXFLAGS) ${OUTEXE}$(OBJDIR)vmovqu_app vmovqu_app.c $(OBJDIR)$(VMOVQU_APP_ASM_OBJ)

$(OBJDIR)ymm_app	: ymm_app.cpp $(OBJDIR)$(YMM_APP_ASM_OBJ)
	$(CXX) $(APP_CXXFLAGS) ${OUTEXE}$(OBJDIR)ymm_app ymm_app.cpp $(OBJDIR)$(YMM_APP_ASM_OBJ)

$(OBJDIR)winapp_exception_context	: winapp_exception_context.cpp  $(OBJDIR)$(YMM_APP_ASM_OBJ)  $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS) $(OBJDIR)$(YMM_APP_ASM_OBJ) $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)

$(OBJDIR)raise_exception_app	: raise_exception_app.cpp  $(OBJDIR)$(YMM_APP_ASM_OBJ)  $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)  $(OBJDIR)$(YMM_APP_ASM_OBJ)  $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)

$(OBJDIR)suspend_context_app_win	: suspend_context_app_win.cpp  $(OBJDIR)$(YMM_APP_ASM_OBJ) $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)  $(OBJDIR)$(YMM_APP_ASM_OBJ) $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)

$(OBJDIR)winapp_exception_ntcontinue : winapp_exception_ntcontinue.cpp $(OBJDIR)$(YMM_APP_ASM_OBJ) $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ) $(OBJDIR)$(NTDLL_LIB)
	${CXX} $(APP_CXXFLAGS) $(NO_LOGO) $(DBG) $(NO_OPTIMIZE)  ${OUTEXE}$@ $< $(APP_CXXLINK_FLAGS)  $(OBJDIR)$(YMM_APP_ASM_OBJ) $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ) $(OBJDIR)$(NTDLL_LIB) 

test: $(TEST_ALL)

test_all:
	make $(AVX_CHECK)
	if $(MACHINE_SUPPORTS_AVX_INSTRUCTIONS); then \
	   echo "Testing AVX tools and applications"; \
	   make test_avx; \
	else \
	   echo "AVX instructions can not be tested on this machine"; \
	   make test_no_avx; \
	fi




	
##======================================================
##  tests
##======================================================

attach_in_sighandler_ymm.test: %.test: $(OBJDIR)attach_in_sighandler_app $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} %.tested %.failed
	./$< -pin $(PIN) -pinarg  -t $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} -o $(OBJDIR)$*.log >$*.out  2>&1
	grep "ymm values are correct" $*.out
	rm $*.failed $*.out $(OBJDIR)$*.log

attach_in_sighandler_ymm1.test: %.test: $(OBJDIR)attach_in_sighandler_app $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} %.tested %.failed
	./$< -test 1 -pin $(PIN) -pinarg  -t $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} -o $(OBJDIR)$*.log >$*.out  2>&1
	grep "ymm values are correct" $*.out
	rm $*.failed $*.out $(OBJDIR)$*.log
                
detach_in_sighandler_ymm.test: %.test: $(OBJDIR)detach_in_sighandler_app $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} %.tested %.failed
	$(PIN) -t $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} -o $(OBJDIR)$*.log -- $(OBJDIR)detach_in_sighandler_app >$*.out  2>&1
	grep "ymm values are correct" $*.out
	rm $*.failed $*.out $(OBJDIR)$*.log

detach_in_sighandler_ymm1.test: %.test: $(OBJDIR)detach_in_sighandler_app $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} %.tested %.failed
	$(PIN) -t $(OBJDIR)attach_detach_jit_tool${PINTOOL_SUFFIX} -o $(OBJDIR)$*.log -- $(OBJDIR)detach_in_sighandler_app -test 1 >$*.out  2>&1
	grep "ymm values are correct" $*.out
	rm $*.failed $*.out $(OBJDIR)$*.log
    
ymm_ref.test : $(OBJDIR)ymm_ref$(PINTOOL_SUFFIX) ymm_ref.tested ymm_ref.failed
	echo "ymm_ref should fail.  Ignore the error."
	-$(PIN) -xyzzy  -logfile ymm_ref.log -t $< --  $(TESTAPP) makefile $<.makefile.copy >  $<.out 2>&1
	grep -q "ymm registers are NOT supported in this instance of Pin" ymm_ref.log
	rm ymm_ref.failed $<.out

ymm_ref_actual.test	: $(OBJDIR)ymm_ref_actual$(PINTOOL_SUFFIX)	ymm_ref_actual.tested ymm_ref_actual.failed	$(OBJDIR)ymm_app	
	$(PIN) -t $< -- ./$(OBJDIR)ymm_app
	rm -f ymm_ref_actual.failed 

vmovdqu.test	: $(OBJDIR)set_ymm_scratches$(PINTOOL_SUFFIX)	vmovqu.tested vmovqu.failed	$(OBJDIR)vmovqu_app	
	$(PIN) -t $< -- ./$(OBJDIR)vmovqu_app
	rm -f vmovqu.failed

vmovqu_noinline.test	: $(OBJDIR)set_ymm_scratches$(PINTOOL_SUFFIX)	vmovqu_noinline.tested vmovqu_noinline.failed	$(OBJDIR)vmovqu_app	
	$(PIN) -xyzzy -inline 0 -t $< -- ./$(OBJDIR)vmovqu_app
	rm -f vmovqu_noinline.failed
	
exception_context_monitor.test	: $(OBJDIR)exception_context_monitor$(PINTOOL_SUFFIX)	exception_context_monitor.tested exception_context_monitor.failed	$(OBJDIR)winapp_exception_context	
	$(PIN) -t $< -- ./$(OBJDIR)winapp_exception_context
	rm -f exception_context_monitor.failed 
	
raise_exception.test	: $(OBJDIR)raise_exception$(PINTOOL_SUFFIX)	raise_exception.tested raise_exception.failed	$(OBJDIR)raise_exception_app	
	$(PIN) -t $< -- ./$(OBJDIR)raise_exception_app
	rm -f raise_exception.failed  

suspend_context_win.test	:	suspend_context.tested suspend_context.failed	$(OBJDIR)suspend_context_app_win	
	$(PIN)  -- ./$(OBJDIR)suspend_context_app_win
	rm -f suspend_context.failed 

exception_ntcontinue.test :	exception_ntcontinue.tested suspend_context.failed	$(OBJDIR)winapp_exception_ntcontinue
	$(PIN)  -- ./$(OBJDIR)winapp_exception_ntcontinue
	rm -f exception_ntcontinue.failed 

##  build rules


$(OBJDIR)attach_in_sighandler_app: attach_in_sighandler_app.cpp $(OBJDIR)fp_save_restore.o
	$(CXX) -O0 --no-inline ${APP_CXXFLAGS} $(DBG) ${NO_COMDAT_FLAG} -c -o $(OBJDIR)attach_in_sighandler_app.o $<
	$(CXX) ${APP_CXXFLAGS} $(DBG) $(OBJDIR)fp_save_restore.o $(OBJDIR)attach_in_sighandler_app.o $(APP_CXXLINK_FLAGS) -lpthread -o $@
        
$(OBJDIR)detach_in_sighandler_app: detach_in_sighandler_app.cpp $(OBJDIR)fp_save_restore.o
	$(CXX) -O0 --no-inline ${APP_CXXFLAGS} $(DBG) ${NO_COMDAT_FLAG} -c -o $(OBJDIR)detach_in_sighandler_app.o $<
	$(CXX) ${APP_CXXFLAGS} $(DBG) $(OBJDIR)fp_save_restore.o $(OBJDIR)detach_in_sighandler_app.o $(APP_CXXLINK_FLAGS) -lpthread -o $@

$(OBJDIR)fp_save_restore.o: fp_save_restore_$(TARGET).s
	$(CXX) ${APP_CXXFLAGS} -c fp_save_restore_$(TARGET).s -o $(OBJDIR)fp_save_restore.o

	
	
$(OBJDIR)%.o : %.cpp
	$(CXX) ${COPT} $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
    
$(TOOLS): $(OBJDIR)%$(PINTOOL_SUFFIX) :	$(OBJDIR)%.o $(PIN_LIBNAMES)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $<	${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)set_ymm_scratches$(PINTOOL_SUFFIX): $(OBJDIR)set_ymm_scratches.o $(OBJDIR)$(SET_YMM_SCRATCHES_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $<	 $(OBJDIR)$(SET_YMM_SCRATCHES_ASM_OBJ) ${PIN_LPATHS} $(PIN_LIBS) $(DBG)


$(OBJDIR)exception_context_monitor$(PINTOOL_SUFFIX): $(OBJDIR)exception_context_monitor.o  $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $<	$(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ) ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

$(OBJDIR)raise_exception$(PINTOOL_SUFFIX): $(OBJDIR)raise_exception.o  $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@  $< $(OBJDIR)$(VERIFY_FP_CONTEXT_ASM_OBJ) ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

## cleaning
clean:
	-rm	-rf	$(OBJDIR) *.tested *.failed	*.obj *.o *.out *.log* *.txt *.pdb


