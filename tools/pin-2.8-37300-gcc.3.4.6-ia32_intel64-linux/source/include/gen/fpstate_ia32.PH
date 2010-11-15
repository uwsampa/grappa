//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|PIN_SYSCALL_API|WINDOWS_SYSCALL_API_UNDOC|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|DEPRECATED_PIN_API|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|BBL_BASIC|ROGUE_BASIC_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|UNDOCUMENTED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


struct FXSAVE
{
    UINT16 _fcw;
    UINT16 _fsw;
    UINT8  _ftw;
    UINT8  _pad1;
    UINT16 _fop;
    UINT32 _fpuip;
    UINT16 _cs;
    UINT16 _pad2;
    UINT32 _fpudp;
    UINT16 _ds;
    UINT16 _pad3;
    UINT32 _mxcsr;
    UINT32 _mxcsrmask;
    UINT8  _st[8 * 16];
    UINT8  _xmm[8 * 16];
    UINT8  _pad4[56 * 4];
};

                                                                  /* DO NOT EDIT */
struct XSAVE_HEADER
{
    UINT64 _mask;
    UINT64 _reserved[7];
};

                                                                  /* DO NOT EDIT */
struct FPSTATE
{
    // fxsave_legacy is applicable on all IA-32 and Intel(R) 64
    // processors
    FXSAVE fxsave_legacy;
    // the following are only applicable on processors that support XSAVE
    struct XSTATE
    {
        XSAVE_HEADER _extendedHeader;
        UINT8  _ymmUpper[8*16];
        UINT8  _pad5[8*16];
    }_xstate;
};

                                                                  /* DO NOT EDIT */
const size_t FPSTATE_SIZE = sizeof(FPSTATE);

                                                                  /* DO NOT EDIT */
const size_t FPSTATE_ALIGNMENT = 64;

                                                                  /* DO NOT EDIT */
const size_t FP_STATE_EXTENDED_HEADER_SIZE = sizeof(XSAVE_HEADER);

                                                                  /* DO NOT EDIT */

