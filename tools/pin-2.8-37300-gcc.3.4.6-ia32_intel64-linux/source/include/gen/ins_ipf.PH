//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|PIN_SYSCALL_API|WINDOWS_SYSCALL_API_UNDOC|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|DEPRECATED_PIN_API|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|BBL_BASIC|ROGUE_BASIC_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|UNDOCUMENTED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


extern MESSAGE_TYPE MessageTypeLogDecodeIpf;

                                                                  /* DO NOT EDIT */
/* DO NOT EDIT */

#define LOG_DECODE_IPF(message)     QMESSAGE(MessageTypeLogDecodeIpf,      string("decode: ")          + message)

extern OPCODE INS_Opcode(INS ins);

                                                                  /* DO NOT EDIT */
extern VOID InitJmpLikeCall(INS call, INS jmp);

                                                                  /* DO NOT EDIT */
extern BBL SimulateRetPush(INS call);

                                                                  /* DO NOT EDIT */
extern BBL SimulateRetPopToMem(INS ret, ADDRINT offset);

                                                                  /* DO NOT EDIT */
extern VOID INS_InvertBr(INS br);

                                                                  /* DO NOT EDIT */
extern VOID INS_EncodeAtAddr(INS ins, ADDRINT addr);

                                                                  /* DO NOT EDIT */
extern USIZE INS_InitBranchProbe(INS dst, ADDRINT src_addr, ADDRINT dst_addr);

                                                                  /* DO NOT EDIT */
extern USIZE INS_MaxProbeSize();

                                                                  /* DO NOT EDIT */
extern REG INS_RegR(INS ins, UINT32 n );

                                                                  /* DO NOT EDIT */
extern VOID INS_RegRSet(INS ins, UINT32 n, REG r );

                                                                  /* DO NOT EDIT */
extern REG INS_RegW(INS ins, UINT32 n );

                                                                  /* DO NOT EDIT */
extern VOID INS_RegWSet(INS ins, UINT32 n, REG r );

                                                                  /* DO NOT EDIT */
extern BOOL INS_RegRContain(const INS ins, const REG reg);

                                                                  /* DO NOT EDIT */
extern BOOL INS_RegWContain(const INS ins, const REG reg);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MaxNumRRegs(INS x);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MaxNumWRegs(INS x);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsStackRead(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsStackWrite(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsIpRelRead(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsIpRelWrite(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsAtomicUpdate(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsPairedLoad(INS ins);

                                                                  /* DO NOT EDIT */
extern CATEGORY INS_Category(INS ins);

                                                                  /* DO NOT EDIT */
extern INT32 INS_Extension(INS ins);

                                                                  /* DO NOT EDIT */
extern string OPCODE_StringShort(UINT32 opcode);

                                                                  /* DO NOT EDIT */
extern string INS_StringShort(INS ins );

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsOriginal(INS ins);

                                                                  /* DO NOT EDIT */
extern string INS_Disassemble(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsBranchOrCall(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsBranch(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsDirectBranchOrCall(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsDirectBranch(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsRotateRegisterBranch(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsIndirectBranchOrCall(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsCall(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_Stutters(INS);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsProcedureCall(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsDirectCall(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsRet(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsIndirectCondBranch(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_HasFallThrough(INS ins);

                                                                  /* DO NOT EDIT */
extern string INS_Mnemonic(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsPrefetch(INS ins);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MemoryReadSize(INS ins);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MemoryWriteSize(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsSyscall(INS ins);

                                                                  /* DO NOT EDIT */
extern SYSCALL_STANDARD INS_SyscallStd(INS ins);

                                                                  /* DO NOT EDIT */
extern PREDICATE INS_GetPredicate(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsPredicated(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsNop(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsMemoryRead(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_HasMemoryRead2(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsMemoryWrite(INS ins);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MemoryOperandCount(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_MemoryOperandIsRead(INS ins, UINT32 memOpIdx);

                                                                  /* DO NOT EDIT */
extern BOOL INS_MemoryOperandIsWritten(INS ins, UINT32 memOpIdx);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MemoryOperandSize(INS ins, UINT32 memOpIdx);

                                                                  /* DO NOT EDIT */
/* DO NOT EDIT */
/* INS_IsRewritableMemOpBaseLimited is now deprecated. See INS_RewriteMemoryOperand
 * for a more powerful interface to memory operand rewriting.
 */
extern PIN_DEPRECATED_API BOOL INS_IsRewritableMemOpBaseLimited(INS ins, MEMORY_TYPE mtype, REG & base);

                                                                  /* DO NOT EDIT */
extern BOOL INS_HasProperty(INS ins, INS_PROP prop);

                                                                  /* DO NOT EDIT */
extern VOID INS_ConvertChkToBrCond(INS ins, REG pred);

                                                                  /* DO NOT EDIT */

