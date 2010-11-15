//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|PIN_SYSCALL_API|WINDOWS_SYSCALL_API_UNDOC|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|DEPRECATED_PIN_API|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|BBL_BASIC|ROGUE_BASIC_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|UNDOCUMENTED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


typedef enum
{
    REG_INVALID_ = 0,
#if !defined(TARGET_DOXYGEN)
    REG_NONE = 1,
    REG_FIRST = 2,

    // immediate operand
    REG_IMM8 = REG_FIRST,
    REG_IMM_BASE = REG_IMM8,
    REG_IMM,
    REG_IMM32,
    REG_IMM_LAST = REG_IMM32,

    // memory operand
    REG_MEM,
    REG_MEM_BASE = REG_MEM,
    REG_MEM_OFF8,
    REG_MEM_OFF32,
    REG_MEM_LAST = REG_MEM_OFF32,

    // memory-address offset operand
    REG_OFF8,
    REG_OFF_BASE = REG_OFF8,
    REG_OFF,
    REG_OFF32,
    REG_OFF_LAST = REG_OFF32,

    REG_MODX,

    // base for all kinds of registers (application, machine, pin)
    REG_RBASE, 

    // Machine registers are individual real registers on the machine
    REG_MACHINE_BASE = REG_RBASE,

    // Application registers are registers used in the application binary
    // Application registers include all machine registers. In addition,
    // they include some aggregrate registers that can be accessed by
    // the application in a single instruction
    // Essentially, application registers = individual machine registers + aggregrate registers
    
    REG_APPLICATION_BASE = REG_RBASE, 

    /* !@ todo: should save scratch mmx and fp registers */
    // The machine registers that form a context. These are the registers
    // that need to be saved in a context switch.
    REG_PHYSICAL_CONTEXT_BEGIN = REG_RBASE,
#endif
    
    REG_GR_BASE = REG_RBASE,
#if defined(TARGET_IA32E)
    // Context registers in the Intel(R) 64 architecture
    REG_RDI = REG_GR_BASE,  ///< rdi
    REG_GDI = REG_RDI,      ///< edi on a 32 bit machine, rdi on 64
    REG_RSI,                ///< rsi
    REG_GSI = REG_RSI,      ///< esi on a 32 bit machine, rsi on 64
    REG_RBP,                ///< rbp
    REG_GBP = REG_RBP,      ///< ebp on a 32 bit machine, rbp on 64
    REG_RSP,                ///< rsp
    REG_STACK_PTR = REG_RSP,///< esp on a 32 bit machine, rsp on 64
    REG_RBX,                ///< rbx
    REG_GBX = REG_RBX,      ///< ebx on a 32 bit machine, rbx on 64
    REG_RDX,                ///< rdx
    REG_GDX = REG_RDX,      ///< edx on a 32 bit machine, rdx on 64
    REG_RCX,                ///< rcx
    REG_GCX = REG_RCX,      ///< ecx on a 32 bit machine, rcx on 64
    REG_RAX,                ///< rax
    REG_GAX = REG_RAX,      ///< eax on a 32 bit machine, rax on 64
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_GR_LAST = REG_R15,

    REG_SEG_BASE,
    REG_SEG_CS = REG_SEG_BASE,
    REG_SEG_SS,
    REG_SEG_DS,
    REG_SEG_ES,
    REG_SEG_FS,
    REG_SEG_GS,
    REG_SEG_LAST = REG_SEG_GS,

    REG_RFLAGS,
    REG_GFLAGS=REG_RFLAGS,
    REG_RIP,
    REG_INST_PTR = REG_RIP,
#else
    // Context registers in the IA-32 architecture
    REG_EDI = REG_GR_BASE,
    REG_GDI = REG_EDI,
    REG_ESI,
    REG_GSI = REG_ESI,
    REG_EBP,
    REG_GBP = REG_EBP,
    REG_ESP,
    REG_STACK_PTR = REG_ESP,
    REG_EBX,
    REG_GBX = REG_EBX,
    REG_EDX,
    REG_GDX = REG_EDX,
    REG_ECX,
    REG_GCX = REG_ECX,
    REG_EAX,
    REG_GAX = REG_EAX,
    REG_GR_LAST = REG_EAX,
    
    REG_SEG_BASE,
    REG_SEG_CS = REG_SEG_BASE,
    REG_SEG_SS,
    REG_SEG_DS,
    REG_SEG_ES,
    REG_SEG_FS,
    REG_SEG_GS,
    REG_SEG_LAST = REG_SEG_GS,

    REG_EFLAGS,
    REG_GFLAGS=REG_EFLAGS,
    REG_EIP,
    REG_INST_PTR = REG_EIP,
#endif
    
    REG_PHYSICAL_CONTEXT_END = REG_INST_PTR,

    // partial registers common to both the IA-32 and Intel(R) 64 architectures.
    REG_AL,
    REG_AH,
    REG_AX,
    
    REG_CL,
    REG_CH,
    REG_CX,
    
    REG_DL,
    REG_DH,
    REG_DX,
    
    REG_BL,
    REG_BH,
    REG_BX,

    REG_BP,
    REG_SI,
    REG_DI,

    REG_SP,
    REG_FLAGS,
    REG_IP,
    
#if defined(TARGET_IA32E)
    // partial registers in the Intel(R) 64 architecture
    REG_EDI,
    REG_DIL,
    REG_ESI,
    REG_SIL,
    REG_EBP,
    REG_BPL,
    REG_ESP,
    REG_SPL,
    REG_EBX,
    REG_EDX,
    REG_ECX,
    REG_EAX,
    REG_EFLAGS,
    REG_EIP,

    REG_R8B,
    REG_R8W,
    REG_R8D,
    REG_R9B,
    REG_R9W,
    REG_R9D,
    REG_R10B,
    REG_R10W,
    REG_R10D,
    REG_R11B,
    REG_R11W,
    REG_R11D,
    REG_R12B,
    REG_R12W,
    REG_R12D,    
    REG_R13B,
    REG_R13W,
    REG_R13D,
    REG_R14B,
    REG_R14W,
    REG_R14D,
    REG_R15B,
    REG_R15W,
    REG_R15D,    
#endif
    
    REG_MM_BASE,
    REG_MM0 = REG_MM_BASE,
    REG_MM1,
    REG_MM2,
    REG_MM3,
    REG_MM4,
    REG_MM5,
    REG_MM6,
    REG_MM7,
    REG_MM_LAST = REG_MM7,
    
    REG_EMM_BASE,
    REG_EMM0 = REG_EMM_BASE,
    REG_EMM1,
    REG_EMM2,
    REG_EMM3,
    REG_EMM4,
    REG_EMM5,
    REG_EMM6,
    REG_EMM7,
    REG_EMM_LAST = REG_EMM7,

    REG_MXT,
    
    REG_XMM_BASE,
    REG_XMM0 = REG_XMM_BASE,
    REG_XMM1,
    REG_XMM2,
    REG_XMM3,
    REG_XMM4,
    REG_XMM5,
    REG_XMM6,
    REG_XMM7,
    
#if defined(TARGET_IA32E)
    // additional xmm registers in the Intel(R) 64 architecture
    REG_XMM8,
    REG_XMM9,
    REG_XMM10,
    REG_XMM11,
    REG_XMM12,
    REG_XMM13,
    REG_XMM14,
    REG_XMM15,
    REG_XMM_LAST = REG_XMM15,
#else    
    REG_XMM_LAST = REG_XMM7,
#endif

    REG_YMM_BASE,
    REG_YMM0 = REG_YMM_BASE,
    REG_YMM1,
    REG_YMM2,
    REG_YMM3,
    REG_YMM4,
    REG_YMM5,
    REG_YMM6,
    REG_YMM7,
    
#if defined(TARGET_IA32E)
    // additional ymm registers in the Intel(R) 64 architecture
    REG_YMM8,
    REG_YMM9,
    REG_YMM10,
    REG_YMM11,
    REG_YMM12,
    REG_YMM13,
    REG_YMM14,
    REG_YMM15,
    REG_YMM_LAST = REG_YMM15,
#else    
    REG_YMM_LAST = REG_YMM7,
#endif
    
    REG_MXCSR,

    // This corresponds to the "orig_eax" register that is visible
    // to some debuggers.
#if defined(TARGET_IA32E)
    REG_ORIG_RAX,
    REG_ORIG_GAX = REG_ORIG_RAX,
#else
    REG_ORIG_EAX,
    REG_ORIG_GAX = REG_ORIG_EAX,
#endif

    REG_DR_BASE,
    REG_DR0 = REG_DR_BASE,
    REG_DR1,
    REG_DR2,
    REG_DR3,
    REG_DR4,
    REG_DR5,
    REG_DR6,
    REG_DR7,
    REG_DR_LAST = REG_DR7,

    REG_CR_BASE,
    REG_CR0 = REG_CR_BASE,
    REG_CR1,
    REG_CR2,
    REG_CR3,
    REG_CR4,
    REG_CR_LAST = REG_CR4,
    
    REG_TSSR,
    
    REG_LDTR,

/*
 --- Not clear if following are needed
    REG_ESR_BASE,
    REG_ESR_LIMIT,
    
    REG_CSR_BASE,
    REG_CSR_LIMIT,
    
    REG_SSR_BASE,
    REG_SSR_LIMIT,
    
    REG_DSR_BASE,
    REG_DSR_LIMIT,
    
    REG_FSR_BASE,
    REG_FSR_LIMIT,
    
    REG_GSR_BASE,
    REG_GSR_LIMIT,
    
    REG_TSSR_BASE,
    REG_TSSR_LIMIT,
    
    REG_LDTR_BASE,
    REG_LDTR_LIMIT,
    
    REG_GDTR_BASE,
    REG_GDTR_LIMIT,
    
    REG_IDTR_BASE,
    REG_IDTR_LIMIT,
*/

    REG_TR_BASE,
    REG_TR = REG_TR_BASE,
    REG_TR3,
    REG_TR4,
    REG_TR5,
    REG_TR6,
    REG_TR7,
    REG_TR_LAST = REG_TR7,
    
    REG_FPST_BASE,
    REG_FP_BASE = REG_FPST_BASE,
    REG_FPCW = REG_FP_BASE,
    REG_FPSW,
    REG_FPTAG,
    REG_FPIP_OFF,
    REG_FPIP_SEL,
    REG_FPOPCODE,
    REG_FPDP_OFF,
    REG_FPDP_SEL,
    REG_FP_LAST = REG_FPDP_SEL,
    
    REG_ST_BASE,
    REG_ST0 = REG_ST_BASE,
    REG_ST1,
    REG_ST2,
    REG_ST3,
    REG_ST4,
    REG_ST5,
    REG_ST6,
    REG_ST7,
#if !defined(TARGET_DOXYGEN)
    REG_ST_LAST = REG_ST7,
    REG_FPST_LAST = REG_ST_LAST,
    REG_MACHINE_LAST = REG_FPST_LAST, /* last machine register */
    
    /* these are the two registers implementing the eflags in pin
       REG_STATUS_FLAGS represents the OF, SF, ZF, AF, PF and CF flags.
       REG_DF_FLAG      represents the DF flag.
       flag splitting is done because the DF flag spilling and filling is rather expensive,
       and the DF flag is not read/written by most instructions - therefore it is
       not necessary to spill/fill it on most instructions that read/write the flags.
       (prior to flag splitting, whenever any of the flags needed to be spilled/filled
       both the DF and all the above status flags were spilled/filled).
       NOTE - this flag splitting is not done if the pushf/popf sequence is being used
              rather than the sahf/lahf sequence (some early Intel64 preocessors do not
              support sahf/lahf instructions). Also the KnobRegFlagsSplit can be used
              to disable the flags splitting when the sahf/lahf sequence is being used
       Flags splitting is not done at the INS operand level - it is done when building
       the vreglist for register allocation. So tools see the architectural flags registers
       in INSs. See the functions MakeRegisterList and REG_InsertReadRegToVreglist to see
       how the split flags are inserted into the vreglist.
       See jit_flags_spillfill_ia32.cpp file comments to learn how they are spilled/filled
     */
    REG_STATUS_FLAGS,
    REG_DF_FLAG,
    
    /* Aggregates of registers (still application regs) */
    REG_AGGREGATE_BASE,
    REG_FPST_ALL = REG_AGGREGATE_BASE,
    REG_AGGREGATE_LAST = REG_FPST_ALL,

    REG_APPLICATION_LAST = REG_AGGREGATE_LAST, /* last register name used by the application */
    
    /* Pin's virtual register names */
    REG_PIN_BASE,
    REG_PIN_GR_BASE = REG_PIN_BASE,

    // ia32-specific Pin gr regs
    REG_PIN_EDI = REG_PIN_GR_BASE,
#if defined(TARGET_IA32)    
    REG_PIN_GDI = REG_PIN_EDI,                  // PIN_GDI == PIN_EDI on 32 bit, PIN_RDI on 64 bit.
#endif
    REG_PIN_ESI,
    REG_PIN_EBP,
    REG_PIN_ESP,
#if defined (TARGET_IA32)
    REG_PIN_STACK_PTR = REG_PIN_ESP,
#endif    
    REG_PIN_EBX,
    REG_PIN_EDX,
#if defined(TARGET_IA32)    
    REG_PIN_GDX = REG_PIN_EDX,                  
#endif
    REG_PIN_ECX,
#if defined(TARGET_IA32)    
    REG_PIN_GCX = REG_PIN_ECX,                  // PIN_GCX == PIN_ECX on 32 bit, PIN_RCX on 64 bit.
#endif
    REG_PIN_EAX,
#if defined(TARGET_IA32)    
    REG_PIN_GAX = REG_PIN_EAX,                  // PIN_GAX == PIN_EAX on 32 bit, PIN_RAX on 64 bit.
#endif
    REG_PIN_AL,
    REG_PIN_AH,
    REG_PIN_AX,
    REG_PIN_CL,
    REG_PIN_CH,
    REG_PIN_CX,
    REG_PIN_DL,
    REG_PIN_DH,
    REG_PIN_DX,
    REG_PIN_BL,
    REG_PIN_BH,
    REG_PIN_BX,
    REG_PIN_BP,
    REG_PIN_SI,
    REG_PIN_DI,
    REG_PIN_SP,

#if defined(TARGET_IA32E)
    // Intel(R) 64 architecture specific pin gr regs
    REG_PIN_RDI,
    REG_PIN_GDI = REG_PIN_RDI,
    REG_PIN_RSI,
    REG_PIN_RBP,
    REG_PIN_RSP,
    
    REG_PIN_STACK_PTR = REG_PIN_RSP,
    
    REG_PIN_RBX,
    REG_PIN_RDX,
    REG_PIN_GDX = REG_PIN_RDX,
    REG_PIN_RCX,
    REG_PIN_GCX = REG_PIN_RCX,
    REG_PIN_RAX,
    REG_PIN_GAX = REG_PIN_RAX,
    REG_PIN_R8,
    REG_PIN_R9,
    REG_PIN_R10,
    REG_PIN_R11,
    REG_PIN_R12,
    REG_PIN_R13,
    REG_PIN_R14,
    REG_PIN_R15,

    REG_PIN_DIL,
    REG_PIN_SIL,
    REG_PIN_BPL,
    REG_PIN_SPL,
    
    REG_PIN_R8B,
    REG_PIN_R8W,
    REG_PIN_R8D,

    REG_PIN_R9B,
    REG_PIN_R9W,
    REG_PIN_R9D,

    REG_PIN_R10B,
    REG_PIN_R10W,
    REG_PIN_R10D,

    REG_PIN_R11B,
    REG_PIN_R11W,
    REG_PIN_R11D,

    REG_PIN_R12B,
    REG_PIN_R12W,
    REG_PIN_R12D,

    REG_PIN_R13B,
    REG_PIN_R13W,
    REG_PIN_R13D,

    REG_PIN_R14B,
    REG_PIN_R14W,
    REG_PIN_R14D,

    REG_PIN_R15B,
    REG_PIN_R15W,
    REG_PIN_R15D,
#endif

    /* ! @todoshould be REG_PIN_THREAD_ID ?*/
    // Every thread is assigned an index so we can implement tls
    REG_THREAD_ID,
    
    REG_SEG_GS_VAL,  // virtual reg holding actual value of gs
    REG_SEG_FS_VAL,  // virtual reg holding actual value of fs

    // ISA-independent gr regs
    REG_PIN_INDIRREG,  // virtual reg holding indirect jmp target value
    REG_PIN_IPRELADDR, // virtual reg holding ip-rel address value
    REG_PIN_SYSENTER_RESUMEADDR, // virtual reg holding the resume address from sysenter
    
    // ISA-independent gr regs holding temporary values
    REG_PIN_T_BASE,
    REG_PIN_T0 = REG_PIN_T_BASE,        
    REG_PIN_T1,        
    REG_PIN_T2,
    REG_PIN_T3,
    REG_PIN_T0L,    // lower 8 bits of temporary register
    REG_PIN_T1L,
    REG_PIN_T2L,
    REG_PIN_T3L,
    REG_PIN_T0W,    // lower 16 bits of temporary register
    REG_PIN_T1W,
    REG_PIN_T2W,
    REG_PIN_T3W,
    REG_PIN_T0D,    // lower 32 bits of temporary register
    REG_PIN_T1D,
    REG_PIN_T2D,
    REG_PIN_T3D,
    REG_PIN_T_LAST = REG_PIN_T3D,

#endif

    // Virtual registers reg holding memory addresses pointed by GS/FS registers
    // These registers are visible for tool writers
    REG_SEG_GS_BASE, ///< Base address for GS segment
    REG_SEG_FS_BASE, ///< Base address for FS segment

    // ISA-independent Pin virtual regs needed for instrumentation
    // These are pin registers visible to the pintool writers.
    REG_INST_BASE,
    REG_INST_SCRATCH_BASE = REG_INST_BASE,  ///< First available scratch register
    REG_INST_G0 = REG_INST_SCRATCH_BASE,    ///< Scratch register used in pintools
    REG_INST_G1,                            ///< Scratch register used in pintools
    REG_INST_G2,                            ///< Scratch register used in pintools
    REG_INST_G3,                            ///< Scratch register used in pintools
    REG_INST_G4,                            ///< Scratch register used in pintools
    REG_INST_G5,                            ///< Scratch register used in pintools
    REG_INST_G6,                            ///< Scratch register used in pintools
    REG_INST_G7,                            ///< Scratch register used in pintools
    REG_INST_G8,                            ///< Scratch register used in pintools
    REG_INST_G9,                            ///< Scratch register used in pintools

    REG_INST_TOOL_FIRST = REG_INST_G0,     
    REG_INST_TOOL_LAST = REG_INST_G9,

    REG_BUF_BASE0,
    REG_BUF_BASE1,
    REG_BUF_BASE2,
    REG_BUF_BASE3,
    REG_BUF_BASE4,
    REG_BUF_BASE5,
    REG_BUF_BASE6,
    REG_BUF_BASE7,
    REG_BUF_BASE8,
    REG_BUF_BASE9,
    REG_BUF_LAST = REG_BUF_BASE9,

    REG_BUF_END0,
    REG_BUF_END1,
    REG_BUF_END2,
    REG_BUF_END3,
    REG_BUF_END4,
    REG_BUF_END5,
    REG_BUF_END6,
    REG_BUF_END7,
    REG_BUF_END8,
    REG_BUF_END9,
    REG_BUF_ENDLAST = REG_BUF_END9,

    REG_INST_SCRATCH_LAST = REG_BUF_ENDLAST,

#if !defined(TARGET_DOXYGEN)
    REG_INST_COND,     // for conditional instrumentation.
    REG_INST_LAST = REG_INST_COND,
    
    // Used for memory rewriting, these are not live outside the region
    // but cannot use general purpose scratch registers, because they're
    // used during instrumentation generation, rather than region generation.
    REG_INST_T0,
    REG_INST_T0L,                               /* Low  8 bits */
    REG_INST_T0W,                               /* Low 16 bits */
    REG_INST_T0D,                               /* Low 32 bits */
    REG_INST_T1,
    REG_INST_T2,
    REG_INST_T3,

    // Used to preserve the predicate value around repped string ops
    REG_INST_PRESERVED_PREDICATE,

    // Used when the AC flag needs to be cleared before analysis routine
    REG_FLAGS_BEFORE_AC_CLEARING,
    
    // Virtual regs used by Pin inside instrumentation bridges.
    // Unlike REG_INST_BASE to REG_INST_LAST, these registers are
    // NOT visible to  Pin clients.
    REG_PIN_BRIDGE_ORIG_SP,    // hold the stack ptr value before the bridge
    REG_PIN_BRIDGE_APP_IP, // hold the application (not code cache) IP to resume
    REG_PIN_BRIDGE_SP_BEFORE_ALIGN, // hold the stack ptr value before the stack alignment
    REG_PIN_BRIDGE_MARSHALLING_FRAME, // hold the address of the marshalled reference registers
    REG_PIN_BRIDGE_CONTEXT_FRAME, // hold the address of the context frame
    REG_PIN_BRIDGE_CONTEXT_ORIG_SP, // hold the sp at which the context was pushed

    REG_PIN_SPILLPTR,  // ptr to the pin spill area
    REG_PIN_GR_LAST = REG_PIN_SPILLPTR,

    // REG_PIN_FLAGS is x86-specific, but since it is not a gr, we put it out of
    // REG_PIN_GR_BASE and REG_PIN_GR_LAST

    /* these are the two registers implementing the PIN flags in pin
       REG_PIN_STATUS_FLAGS represents the OF, SF, ZF, AF, PF and CF flags.
       REG_PIN_DF_FLAG      represents the DF flag.
     */
    REG_PIN_STATUS_FLAGS,
    REG_PIN_DF_FLAG,

    /* REG_PIN_FLAGS is used only in the case when the pushf/popf sequence is used
       for flags spill/fill rather than the sahf/lahf sequence.
     */
    REG_PIN_FLAGS,

    REG_PIN_XMM_BASE,
    REG_PIN_XMM0 = REG_PIN_XMM_BASE,
    REG_PIN_XMM1,
    REG_PIN_XMM2,
    REG_PIN_XMM3,
    REG_PIN_XMM4,
    REG_PIN_XMM5,
    REG_PIN_XMM6,
    REG_PIN_XMM7,
    REG_PIN_XMM8,
    REG_PIN_XMM9,
    REG_PIN_XMM10,
    REG_PIN_XMM11,
    REG_PIN_XMM12,
    REG_PIN_XMM13,
    REG_PIN_XMM14,
    REG_PIN_XMM15,

    REG_PIN_YMM_BASE,
    REG_PIN_YMM0 = REG_PIN_YMM_BASE,
    REG_PIN_YMM1,
    REG_PIN_YMM2,
    REG_PIN_YMM3,
    REG_PIN_YMM4,
    REG_PIN_YMM5,
    REG_PIN_YMM6,
    REG_PIN_YMM7,
    REG_PIN_YMM8,
    REG_PIN_YMM9,
    REG_PIN_YMM10,
    REG_PIN_YMM11,
    REG_PIN_YMM12,
    REG_PIN_YMM13,
    REG_PIN_YMM14,
    REG_PIN_YMM15,
    REG_PIN_LAST = REG_PIN_YMM15,
#endif
    REG_LAST


} REG;

                                                                  /* DO NOT EDIT */
/* DO NOT EDIT */

/*! @ingroup REG_BASIC_CPU_IA32 */
GLOBALCONST ADDRINT AC_FLAG_BIT = 0x40000;

const ADDRINT NUM_PHYSICAL_REGS = REG_PHYSICAL_CONTEXT_END - REG_PHYSICAL_CONTEXT_BEGIN + 1;

                                                                  /* DO NOT EDIT */
const ADDRINT NUM_SCRATCH_REGS = REG_INST_SCRATCH_LAST - REG_INST_SCRATCH_BASE + 1;

                                                                  /* DO NOT EDIT */
const ADDRINT NUM_SPECIAL_REGS = 2 + NUM_SCRATCH_REGS;

                                                                  /* DO NOT EDIT */
const ADDRINT NUM_CONTEXT_INT_REGS = NUM_PHYSICAL_REGS + NUM_SPECIAL_REGS;

                                                                  /* DO NOT EDIT */
const ADDRINT ARCH_STATE_SIZE = (NUM_PHYSICAL_REGS + NUM_SPECIAL_REGS)*sizeof(ADDRINT) + 
                                      (FPSTATE_SIZE // because CONTEXT size must
                                                         // be at least as big as 
                                                         // CONTEXT_CANONICAL size, 
                                                         // and CONTEXT_CANONICAL._fpstate is used
                                      + FPSTATE_ALIGNMENT);

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_gr(REG reg){ return (reg >= REG_GR_BASE) && (reg <= REG_GR_LAST);}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_fr(REG reg)
{
    return
        ((reg >= REG_FPST_BASE) && (reg <= REG_FPST_LAST)) ||
        ((reg >= REG_XMM_BASE) && (reg <= REG_XMM_LAST)) ||
        ((reg >= REG_YMM_BASE) && (reg <= REG_YMM_LAST)) ||
        reg == REG_MXCSR;
}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_br(REG reg)  { return FALSE; }

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_gr64(REG reg)
{
#if defined(TARGET_IA32E)
    // all gr on Intel(R) 64 architectures are 64-bits
    return REG_is_gr(reg);
#else
    // no 64-bit gr on x86
    return FALSE;
#endif
}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_gr32(REG reg)
{
    switch (reg)
    {
      case REG_EDI:
      case REG_ESI:
      case REG_EBP:
      case REG_ESP:
      case REG_EBX:
      case REG_EDX:
      case REG_ECX:
      case REG_EAX:
#if defined(TARGET_IA32E)
      case REG_R8D:
      case REG_R9D:
      case REG_R10D:
      case REG_R11D:
      case REG_R12D:
      case REG_R13D:
      case REG_R14D:
      case REG_R15D:    
#endif        
        return TRUE;
      default:
        return FALSE;
    }
}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_gr16(REG reg)
{
    switch (reg)
    {
      case REG_DI:
      case REG_SI:
      case REG_BP:
      case REG_SP:
      case REG_BX:
      case REG_DX:
      case REG_CX:
      case REG_AX:
#if defined(TARGET_IA32E)
      case REG_R8W:
      case REG_R9W:
      case REG_R10W:
      case REG_R11W:
      case REG_R12W:
      case REG_R13W:
      case REG_R14W:
      case REG_R15W:    
#endif        
        return TRUE;
      default:
        return FALSE;
    }
}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_gr8(REG reg)
{
    switch (reg)
    {
      case REG_AL:
      case REG_AH:
      case REG_BL:
      case REG_BH:
      case REG_CL:
      case REG_CH:
      case REG_DL:
      case REG_DH:
#if defined(TARGET_IA32E)
      case REG_SIL:
      case REG_DIL:
      case REG_SPL:
      case REG_BPL:
      case REG_R8B:
      case REG_R9B:
      case REG_R10B:
      case REG_R11B:
      case REG_R12B:
      case REG_R13B:
      case REG_R14B:
      case REG_R15B:    
#endif        
        return TRUE;
      default:
        return FALSE;
    }
}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_seg(REG reg){ return (reg >= REG_SEG_BASE) && (reg <= REG_SEG_LAST);}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_mm(REG reg){ return (reg >= REG_MM_BASE) && (reg <= REG_MM_LAST);}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_xmm(REG reg){ return (reg >= REG_XMM_BASE) && (reg <= REG_XMM_LAST);}

                                                                  /* DO NOT EDIT */
inline BOOL REG_is_ymm(REG reg){ return (reg >= REG_YMM_BASE) && (reg <= REG_YMM_LAST);}

                                                                  /* DO NOT EDIT */
typedef enum
{
    REGNAME_LAST
}REGNAME;

                                                                  /* DO NOT EDIT */
inline REG REG_INVALID() {return REG_INVALID_;}

                                                                  /* DO NOT EDIT */
inline BOOL REG_valid(REG reg){ return reg != REG_INVALID();}

                                                                  /* DO NOT EDIT */
extern BOOL REG_is_pin64(REG reg);

                                                                  /* DO NOT EDIT */
extern string REG_StringShort(REG reg);

                                                                  /* DO NOT EDIT */
extern REG REG_FullRegName(const REG reg);

                                                                  /* DO NOT EDIT */

