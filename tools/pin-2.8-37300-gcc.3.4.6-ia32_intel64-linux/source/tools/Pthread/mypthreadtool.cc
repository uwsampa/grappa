#include "mypthreadtool.h"

using namespace PinPthread;

/* ================================================================== */

int main(UINT32 argc, char** argv) 
{
    Init(argc, argv);
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    IMG_AddInstrumentFunction(FlagImg, 0);
    RTN_AddInstrumentFunction(FlagRtn, 0);
    TRACE_AddInstrumentFunction(FlagTrace, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    return 0;
}

/* ================================================================== */

namespace PinPthread
{

/* ------------------------------------------------------------------ */
/* Initalization & Clean Up                                           */
/* ------------------------------------------------------------------ */

VOID Init(UINT32 argc, char** argv) 
{
    pthreadsim = new PthreadSim();
}

VOID Fini(INT32 code, VOID* v) 
{
    delete pthreadsim;
}

/* ------------------------------------------------------------------ */
/* Instrumentation Routines                                           */
/* ------------------------------------------------------------------ */

VOID FlagImg(IMG img, VOID* v) 
{
    RTN rtn;
    rtn = RTN_FindByName(img, "__kmp_get_global_thread_id");
    if (rtn != RTN_Invalid()) 
    {
        RTN_ReplaceWithUninstrumentedRoutine(rtn, (AFUNPTR)CallPthreadSelf);
    }
    rtn = RTN_FindByName(img, "__kmp_check_stack_overlap");
    if (rtn != RTN_Invalid()) 
    {
        RTN_ReplaceWithUninstrumentedRoutine(rtn, (AFUNPTR)DummyFunc);
    }
}

VOID FlagRtn(RTN rtn, VOID* v) 
{
    RTN_Open(rtn);
    const string* rtn_name = new string(RTN_Name(rtn));
#if VERYVERBOSE
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)PrintRtnName,
                   IARG_PTR, rtn_name,
                   IARG_END);
#endif
    if (rtn_name->find("pthread_attr_destroy") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrDestroy,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_getdetachstate") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrGetdetachstate,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_getstackaddr") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrGetstackaddr,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_getstacksize") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrGetstacksize,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_getstack") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrGetstack,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_G_ARG2_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_init") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER,(AFUNPTR)CallPthreadAttrInit,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_setdetachstate") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrSetdetachstate,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_setstackaddr") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrSetstackaddr,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_G_ARG2_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_setstacksize") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrSetstacksize,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_attr_setstack") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadAttrSetstack,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_G_ARG2_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cancel") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCancel,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cleanup_pop") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadCleanupPop,
                       IARG_CONTEXT,
                       IARG_G_ARG0_CALLEE,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cleanup_push") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadCleanupPush,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_condattr_destroy") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondattrDestroy,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_condattr_init") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondattrInit,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cond_broadcast") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondBroadcast,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cond_destroy") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondDestroy,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cond_init") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondInit,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cond_signal") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondSignal,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cond_timedwait") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondTimedwait,
                       IARG_CHECKPOINT,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_G_ARG2_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_cond_wait") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadCondWait,
                       IARG_CHECKPOINT,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_create") != string::npos)
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadCreate,
                       IARG_CONTEXT,
                       IARG_CHECKPOINT,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_G_ARG2_CALLEE,
                       IARG_G_ARG3_CALLEE,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_detach") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadDetach,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_equal") != string::npos)
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadEqual,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_exit") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadExit,
                       IARG_CONTEXT,
                       IARG_G_ARG0_CALLEE,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_getattr") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadGetattr,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if ((rtn_name->find("pthread_getspecific") != string::npos) ||
             (rtn_name->find("libc_tsd_get") != string::npos))
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadGetspecific,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_join") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadJoin,
                       IARG_CHECKPOINT,
                       IARG_CONTEXT,
                       IARG_G_ARG0_CALLEE,    
                       IARG_G_ARG1_CALLEE,                       
                       IARG_END);
    }
    else if (rtn_name->find("pthread_key_create") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadKeyCreate,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE, 
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_key_delete") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadKeyDelete,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_kill") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadKill,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutexattr_destroy") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexattrDestroy,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if ((rtn_name->find("pthread_mutexattr_getkind") != string::npos) ||
             (rtn_name->find("pthread_mutexattr_gettype") != string::npos))
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexattrGetkind,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutexattr_init") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexattrInit,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if ((rtn_name->find("pthread_mutexattr_setkind") != string::npos) ||
             (rtn_name->find("pthread_mutexattr_settype") != string::npos))
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexattrSetkind,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutex_destroy") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexDestroy,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutex_init") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexInit,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutex_lock") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadMutexLock,
                       IARG_CHECKPOINT,
                       IARG_G_ARG0_CALLEE,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutex_trylock") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexTrylock,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_mutex_unlock") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadMutexUnlock,
                       IARG_G_ARG0_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_once") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadOnce,
                       IARG_CONTEXT,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_self") != string::npos)
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadSelf,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_setcancelstate") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadSetcancelstate,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_setcanceltype") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadSetcanceltype,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if ((rtn_name->find("pthread_setspecific") != string::npos) ||
             (rtn_name->find("libc_tsd_set") != string::npos))
    {
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CallPthreadSetspecific,
                       IARG_G_ARG0_CALLEE,
                       IARG_G_ARG1_CALLEE,
                       IARG_RETURN_REGS, REG_GAX,
                       IARG_END);
    }
    else if (rtn_name->find("pthread_testcancel") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CallPthreadTestcancel,
                       IARG_CONTEXT,
                       IARG_END);
    }
    else if (((rtn_name->find("__kmp") != string::npos) &&
              (rtn_name->find("yield") != string::npos)) ||
             (rtn_name->find("__sleep") != string::npos) ||
             (rtn_name->find("__kmp_wait_sleep") != string::npos))
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)DoContextSwitch,
                       IARG_CHECKPOINT,
                       IARG_END);
    }
    else if ((rtn_name->find("__pthread_return_void") != string::npos) ||
             (rtn_name->find("pthread_mutex_t") != string::npos) ||
             (rtn_name->find("pthread_atfork") != string::npos))
    {
    }
    else if ((rtn_name->find("pthread") != string::npos) ||
             (rtn_name->find("sigwait") != string::npos) ||
             (rtn_name->find("tsd") != string::npos) ||
             ((rtn_name->find("fork") != string::npos) &&
              (rtn_name->find("__kmp") == string::npos)))
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)WarnNYI,
                       IARG_PTR, rtn_name,
                       IARG_END);
    }
    RTN_Close(rtn);
}

VOID FlagTrace(TRACE trace, VOID* v) 
{
    if (TRACE_Address(trace) == (ADDRINT)pthread_exit) 
    {
        TRACE_InsertCall(trace, IPOINT_BEFORE, (AFUNPTR)CallPthreadExit,
                         IARG_CONTEXT,
                         IARG_G_ARG0_CALLEE,
                         IARG_END);
    }
    else if (!INS_IsAddedForFunctionReplacement(BBL_InsHead(TRACE_BblHead(trace)))) 
    {
        for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) 
        {
            for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) 
            {
                if (INS_IsCall(ins) && !INS_IsDirectBranchOrCall(ins))            // indirect call
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessCall,
                                   IARG_BRANCH_TARGET_ADDR,
                                   IARG_G_ARG0_CALLER,
                                   IARG_G_ARG1_CALLER,
                                   IARG_BOOL, false,
                                   IARG_END);
                }
                else if (INS_IsDirectBranchOrCall(ins))                           // tail call or conventional call
                {
                    ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
                    RTN src_rtn = INS_Rtn(ins);
                    RTN dest_rtn = RTN_FindByAddress(target);
                    if (INS_IsCall(ins) || (src_rtn != dest_rtn)) 
                    {
                        BOOL tailcall = !INS_IsCall(ins);
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessCall,
                                       IARG_ADDRINT, target,
                                       IARG_G_ARG0_CALLER,
                                       IARG_G_ARG1_CALLER,
                                       IARG_BOOL, tailcall,
                                       IARG_END);
                    }
                }
                else if (INS_IsRet(ins))                                          // return
                {
                    RTN rtn = INS_Rtn(ins);
                    if (RTN_Valid(rtn) && (RTN_Name(rtn) != "_dl_runtime_resolve")) 
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessReturn,
                                       IARG_PTR, new string(RTN_Name(rtn)),
                                       IARG_END);
                    }
                }
                /* context switching upon all memory instructions */
                if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins))
                {
                    if (((INS_Address(ins) - (ADDRINT)StartThreadFunc) < 0) ||
                        ((INS_Address(ins) - (ADDRINT)StartThreadFunc) > 8 * sizeof(ADDRINT)))
                    {
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)DoContextSwitch,
                                       IARG_CHECKPOINT,
                                       IARG_END);
                    }
                }
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Pthread Hooks                                                      */
/* ------------------------------------------------------------------ */

int CallPthreadAttrDestroy(ADDRINT _attr) 
{
    return PthreadAttr::pthread_attr_destroy((pthread_attr_t*)_attr);
}

int CallPthreadAttrGetdetachstate(ADDRINT _attr, ADDRINT _detachstate) 
{
    return PthreadAttr::pthread_attr_getdetachstate((pthread_attr_t*)_attr, (int*)_detachstate);
}

int CallPthreadAttrGetstack(ADDRINT _attr, ADDRINT _stackaddr, ADDRINT _stacksize) 
{
    return PthreadAttr::pthread_attr_getstack((pthread_attr_t*)_attr,
                                              (void**)_stackaddr, (size_t*)_stacksize);
}

int CallPthreadAttrGetstackaddr(ADDRINT _attr, ADDRINT _stackaddr) 
{
    return PthreadAttr::pthread_attr_getstackaddr((pthread_attr_t*)_attr, (void**)_stackaddr);
}

int CallPthreadAttrGetstacksize(ADDRINT _attr, ADDRINT _stacksize) 
{
    return PthreadAttr::pthread_attr_getstacksize((pthread_attr_t*)_attr, (size_t*)_stacksize);
}

int CallPthreadAttrInit(ADDRINT _attr) 
{
    return PthreadAttr::pthread_attr_init((pthread_attr_t*)_attr);
}

int CallPthreadAttrSetdetachstate(ADDRINT _attr, ADDRINT _detachstate) 
{
    return PthreadAttr::pthread_attr_setdetachstate((pthread_attr_t*)_attr, (int)_detachstate);
}

int CallPthreadAttrSetstack(ADDRINT _attr, ADDRINT _stackaddr, ADDRINT _stacksize) 
{
    return PthreadAttr::pthread_attr_setstack((pthread_attr_t*)_attr,
                                              (void*)_stackaddr, (size_t)_stacksize);
}

int CallPthreadAttrSetstackaddr(ADDRINT _attr, ADDRINT _stackaddr) 
{
    return PthreadAttr::pthread_attr_setstackaddr((pthread_attr_t*)_attr, (void*)_stackaddr);
}

int CallPthreadAttrSetstacksize(ADDRINT _attr, ADDRINT _stacksize) 
{
    return PthreadAttr::pthread_attr_setstacksize((pthread_attr_t*)_attr, (size_t)_stacksize);
}

int CallPthreadCancel(ADDRINT _thread) 
{
    return pthreadsim->pthread_cancel((pthread_t)_thread);
}

VOID CallPthreadCleanupPop(CONTEXT* ctxt, ADDRINT _execute) 
{
    pthreadsim->pthread_cleanup_pop_((int)_execute, ctxt);
}

VOID CallPthreadCleanupPush(ADDRINT _routine, ADDRINT _arg) 
{
    pthreadsim->pthread_cleanup_push_(_routine, _arg);
}

int CallPthreadCondattrDestroy(ADDRINT _attr) 
{
    return 0;
}

int CallPthreadCondattrInit(ADDRINT _attr) 
{
    return 0;
}

int CallPthreadCondBroadcast(ADDRINT _cond) 
{
    return pthreadsim->pthread_cond_broadcast((pthread_cond_t*)_cond);
}

int CallPthreadCondDestroy(ADDRINT _cond) 
{
    return pthreadsim->pthread_cond_destroy((pthread_cond_t*)_cond);
}

int CallPthreadCondInit(ADDRINT _cond, ADDRINT _condattr) 
{
    return PthreadCond::pthread_cond_init((pthread_cond_t*)_cond, (pthread_condattr_t*)_condattr);
}

int CallPthreadCondSignal(ADDRINT _cond) 
{
    return pthreadsim->pthread_cond_signal((pthread_cond_t*)_cond);
}

VOID CallPthreadCondTimedwait(CHECKPOINT* chkpt, ADDRINT _cond, ADDRINT _mutex, ADDRINT _abstime) 
{
    pthreadsim->pthread_cond_timedwait((pthread_cond_t*)_cond, (pthread_mutex_t*)_mutex,
                                       (const struct timespec*)_abstime, chkpt);
}

VOID CallPthreadCondWait(CHECKPOINT* chkpt, ADDRINT _cond, ADDRINT _mutex) 
{
    pthreadsim->pthread_cond_wait((pthread_cond_t*)_cond, (pthread_mutex_t*)_mutex, chkpt);
}

VOID CallPthreadCreate(CONTEXT* ctxt, CHECKPOINT* chkpt,
                       ADDRINT _thread, ADDRINT _attr, ADDRINT _func, ADDRINT _arg) 
{
    pthreadsim->pthread_create((pthread_t*)_thread, (pthread_attr_t*)_attr,
                               ctxt, _func, _arg, chkpt);
}

int CallPthreadDetach(ADDRINT _th) 
{
    return pthreadsim->pthread_detach((pthread_t)_th);
}

int CallPthreadEqual(ADDRINT _thread1, ADDRINT _thread2) 
{
    return pthreadsim->pthread_equal((pthread_t)_thread1, (pthread_t)_thread2);
}

VOID CallPthreadExit(CONTEXT* ctxt, ADDRINT _retval)  
{
    pthreadsim->pthread_exit((void*)_retval, ctxt);
}

int CallPthreadGetattr(ADDRINT _th, ADDRINT _attr) 
{
    return pthreadsim->pthread_getattr((pthread_t)_th, (pthread_attr_t*)_attr);
}

VOID* CallPthreadGetspecific(ADDRINT _key) 
{
    return pthreadsim->pthread_getspecific((pthread_key_t)_key);
}

VOID CallPthreadJoin(CHECKPOINT* chkpt, CONTEXT* ctxt,
                     ADDRINT _th, ADDRINT _thread_return)
{
    pthreadsim->pthread_join((pthread_t)_th, (void**)_thread_return, chkpt, ctxt);
}

int CallPthreadKeyCreate(ADDRINT _key, ADDRINT _func) 
{
    return pthreadsim->pthread_key_create((pthread_key_t*)_key, (void(*)(void*))_func);
}

int CallPthreadKeyDelete(ADDRINT _key) 
{
    return pthreadsim->pthread_key_delete((pthread_key_t)_key);
}

int CallPthreadKill(ADDRINT _thread, ADDRINT _signo) 
{
    return pthreadsim->pthread_kill((pthread_t)_thread, (int)_signo);
}

int CallPthreadMutexattrDestroy(ADDRINT _attr) 
{
    return PthreadMutexAttr::pthread_mutexattr_destroy((pthread_mutexattr_t*) _attr);
}

int CallPthreadMutexattrGetkind(ADDRINT _attr, ADDRINT _kind) 
{
    return PthreadMutexAttr::pthread_mutexattr_getkind((pthread_mutexattr_t*)_attr, (int*)_kind);
}

int CallPthreadMutexattrInit(ADDRINT _attr) 
{
    return PthreadMutexAttr::pthread_mutexattr_init((pthread_mutexattr_t*)_attr);
}

int CallPthreadMutexattrSetkind(ADDRINT _attr, ADDRINT _kind) 
{
    return PthreadMutexAttr::pthread_mutexattr_setkind((pthread_mutexattr_t*)_attr, (int)_kind);
}

int CallPthreadMutexDestroy(ADDRINT _mutex) 
{
    return PthreadMutex::pthread_mutex_destroy((pthread_mutex_t*)_mutex);
}

int CallPthreadMutexInit(ADDRINT _mutex, ADDRINT _mutexattr) 
{
    return PthreadMutex::pthread_mutex_init((pthread_mutex_t*)_mutex, (pthread_mutexattr_t*)_mutexattr);
}

VOID CallPthreadMutexLock(CHECKPOINT* chkpt, ADDRINT _mutex) 
{
    pthreadsim->pthread_mutex_lock((pthread_mutex_t*)_mutex, chkpt);
}

int CallPthreadMutexTrylock(ADDRINT _mutex) 
{
    return pthreadsim->pthread_mutex_trylock((pthread_mutex_t*)_mutex);
}

int CallPthreadMutexUnlock(ADDRINT _mutex) 
{
    return pthreadsim->pthread_mutex_unlock((pthread_mutex_t*)_mutex);
}

VOID CallPthreadOnce(CONTEXT* ctxt, ADDRINT _oncecontrol, ADDRINT _initroutine) 
{
    PthreadOnce::pthread_once((pthread_once_t*)_oncecontrol, _initroutine, ctxt);
}

pthread_t CallPthreadSelf() 
{
    return pthreadsim->pthread_self();
}

int CallPthreadSetcancelstate(ADDRINT _state, ADDRINT _oldstate) 
{
    return pthreadsim->pthread_setcancelstate((int)_state, (int*)_oldstate);
}

int CallPthreadSetcanceltype(ADDRINT _type, ADDRINT _oldtype) 
{
    return pthreadsim->pthread_setcanceltype((int)_type, (int*)_oldtype);
}

int CallPthreadSetspecific(ADDRINT _key, ADDRINT _pointer) 
{
    return pthreadsim->pthread_setspecific((pthread_key_t)_key, (VOID*)_pointer);
}

VOID CallPthreadTestcancel(CONTEXT* ctxt) 
{
    pthreadsim->pthread_testcancel(ctxt);
}

/* ------------------------------------------------------------------ */
/* Thread-Safe Memory Allocation Support                              */
/* ------------------------------------------------------------------ */

VOID ProcessCall(ADDRINT target, ADDRINT arg0, ADDRINT arg1, BOOL tailcall) 
{
    PIN_LockClient();
    RTN rtn = RTN_FindByAddress(target);
    PIN_UnlockClient();
    if (RTN_Valid(rtn)) 
    {
        pthreadsim->threadsafemalloc(true, tailcall, new string(RTN_Name(rtn)));
    }
}

VOID ProcessReturn(const string* rtn_name) 
{
    ASSERTX(rtn_name != NULL);
    pthreadsim->threadsafemalloc(false, false, rtn_name);
}

/* ------------------------------------------------------------------ */
/* Thread Scheduler                                                   */
/* ------------------------------------------------------------------ */

INT32 InMtMode() 
{
    return pthreadsim->inmtmode();
}

VOID DoContextSwitch(CHECKPOINT* chkpt) 
{
    pthreadsim->docontextswitch(chkpt);
}

/* ------------------------------------------------------------------ */
/* Debugging Support                                                  */
/* ------------------------------------------------------------------ */

VOID WarnNYI(const string* rtn_name) 
{
    std::cout << "NYI: " << *rtn_name << "\n" << flush;
    ASSERTX(0);
}

VOID PrintRtnName(const string* rtn_name) 
{
    std::cout << "RTN " << *rtn_name << "\n" << flush;
}

} // namespace PinPthread

