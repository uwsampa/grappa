//Groups: @ingroup\s+(API_REF|KNOBS|IMG_BASIC_API|INS_BASIC_API|INS_INST_API|INS_BASIC_API_GEN_IA32|INS_BASIC_API_IA32|INS_BASIC_API_IPF|INS_MOD_API_GEN_IA32|SEC_BASIC_API|RTN_BASIC_API|REG_BASIC_API|REG_CPU_GENERIC|REG_CPU_IPF|REG_CPU_IA32|TRACE_BASIC_API|BBL_BASIC_API|SYM_BASIC_API|MISC_PRINT|MISC_PARSE|KNOB_API|KNOB_BASIC|KNOB_PRINT|LOCK|PIN_CONTROL|BUFFER_API|PROTO_API|PIN_PROCESS_API|PIN_THREAD_API|PIN_SYSCALL_API|WINDOWS_SYSCALL_API_UNDOC|DEBUG_API|ERROR_FILE_BASIC|TYPE_BASE|INSTLIB|ALARM|CODECACHE_API|CHILD_PROCESS_API|UTILS|IPF_UTILS_API|MISC|CONTEXT_API|PHYSICAL_CONTEXT_API|EXCEPTION_API|APPDEBUG_API|BUFFER_API|PROTO|INST_ARGS|DEPRECATED_PIN_API|INTERNAL_EXCEPTION_PRIVATE_UNDOCUMENTED|PIN_THREAD_PRIVATE|CHILD_PROCESS_INTERNAL|BBL_BASIC|ROGUE_BASIC_API|INS_BASIC_API_GEN_IPF|MESSAGE_TYPE|MESSAGE_BASIC|ERRFILE|MISC_BASIC|ITC_INST_API|CONTEXT_API_UNDOC|EXCEPTION_API_UNDOC|UNDOCUMENTED_PIN_API|OPIN|TRACE_VERSIONS
/* PIN API */

/* THIS FILE IS AUTOMAGICALLY GENERATED - DO NOT CHANGE DIRECTLY*/


extern USIZE INS_MaxProbeSize();

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MaxNumRRegs(INS x);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MaxNumWRegs(INS x);

                                                                  /* DO NOT EDIT */
extern BOOL INS_RegRContain(const INS ins, const REG reg);

                                                                  /* DO NOT EDIT */
extern BOOL INS_RegWContain(const INS ins, const REG reg);

                                                                  /* DO NOT EDIT */
extern BOOL INS_FullRegRContain(const INS ins, const REG reg);

                                                                  /* DO NOT EDIT */
extern BOOL INS_FullRegWContain(const INS ins, const REG reg);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsStackRead(const INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsStackWrite(const INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsIpRelRead(const INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsIpRelWrite(const INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsPredicated(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_HasRealRep(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_IsOriginal(INS ins);

                                                                  /* DO NOT EDIT */
extern string INS_Disassemble(INS ins);

                                                                  /* DO NOT EDIT */
/* DO NOT EDIT */
/* INS_MemoryOffset is now deprecated. Use 
 * INS_MemoryDisplacement() or INS_MemoryDisplacementSigned() instead.
 */
extern PIN_DEPRECATED_API ADDRINT INS_MemoryOffset(INS ins);

                                                                  /* DO NOT EDIT */
extern ADDRINT INS_MemoryDisplacement(INS ins);

                                                                  /* DO NOT EDIT */
extern ADDRDELTA INS_MemoryDisplacementSigned(INS ins);

                                                                  /* DO NOT EDIT */
extern REG INS_MemoryBaseReg(INS ins);

                                                                  /* DO NOT EDIT */
extern REG INS_MemoryIndexReg(INS ins);

                                                                  /* DO NOT EDIT */
extern UINT32 INS_MemoryScale(INS ins);

                                                                  /* DO NOT EDIT */
extern BOOL INS_HasProperty(INS ins, INS_PROP prop);

                                                                  /* DO NOT EDIT */
extern BOOL INS_ChangeReg(const INS ins, const REG old_reg, const REG new_reg, const BOOL as_read);

                                                                  /* DO NOT EDIT */

