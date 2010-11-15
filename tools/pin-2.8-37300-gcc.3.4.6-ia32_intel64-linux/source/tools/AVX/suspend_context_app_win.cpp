/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2010 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*! @file
 * This test application verifies that Pin on Windows correctly handles 
 * system calls that get/set context of a suspended thread.
 */

#define _WIN32_WINNT 0x0400 // Needed for SignalObjectAndWait()

#include <string>
#include <iostream>
#include <windows.h>
#include "context_ex.h"
using namespace std;

typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;

#include "xsave_type.h"
#include "fp_context_manip.h"

#ifdef TARGET_IA32
typedef UINT32 ADDRINT;
#else
typedef UINT64 ADDRINT;
#endif

class WINDOWS_EXTCTXT
{
  public:
    WINDOWS_EXTCTXT();
    VOID SetContextFlags(DWORD contextFlags)  {m_extContext.winCtxt.ContextFlags = contextFlags;}
    BOOL HasContextFlags(DWORD contextFlags)  {return ((m_extContext.winCtxt.ContextFlags&contextFlags) == contextFlags);}
    CONTEXT *GetLegacyContext(){return (&m_extContext.winCtxt); }
    YMMCONTEXT *GetYmmContext()
    {
        // The PCONTEXT_EX is always just after the legacy CONTEXT
        PCONTEXT_EX extendedContext =  (PCONTEXT_EX)((&(m_extContext.winCtxt))+1);
        PXSTATE xStateInfo = reinterpret_cast<PXSTATE>((reinterpret_cast<ADDRINT>(extendedContext)) + extendedContext->XState.Offset);
        return (&(xStateInfo->YmmContext));
    }

    struct  {
        CONTEXT     winCtxt;
        CONTEXT_EX  contextEx;
        __declspec(align(64)) XSTATE buffer;
    } m_extContext;
};

WINDOWS_EXTCTXT::WINDOWS_EXTCTXT()
{
    m_extContext.contextEx.All.Length = sizeof(m_extContext);
    m_extContext.contextEx.All.Offset = -1 * static_cast<LONG>(reinterpret_cast<ADDRINT>(&m_extContext.contextEx) - reinterpret_cast<ADDRINT>(&m_extContext.winCtxt));
    m_extContext.contextEx.Legacy.Length = sizeof(CONTEXT);
    m_extContext.contextEx.Legacy.Offset = m_extContext.contextEx.All.Offset;
    m_extContext.contextEx.XState.Length = sizeof(m_extContext.buffer);
    m_extContext.contextEx.XState.Offset = static_cast<LONG>(reinterpret_cast<ADDRINT>(&m_extContext.buffer) - reinterpret_cast<ADDRINT>(&m_extContext.contextEx));
}


//==========================================================================
// Context manipulation utilities
//==========================================================================
#if defined(TARGET_IA32)




const DWORD YMM_CONTEXT_FLAG = CONTEXT_EXTENDED_REGISTERS | CONTEXT_XSTATE; 

#elif defined(TARGET_IA32E)



const DWORD YMM_CONTEXT_FLAG = CONTEXT_FLOATING_POINT | CONTEXT_XSTATE; 

#endif
const size_t YmmRegSize = 16;

// Auto-reset event. It is signaled when a thread is about to start. 
HANDLE StartEvent;

// Manual-reset event. It is signaled when a thread is allowed to start.
HANDLE AllowStartEvent;



//==========================================================================
// Printing utilities
//==========================================================================
string UnitTestName("suspend_context_win");
string FunctionTestName;

static void StartFunctionTest(const string & functionTestName)
{
    if (FunctionTestName != "")
    {
        cerr << UnitTestName << "[" << FunctionTestName  << "] Success" << endl;
    }
    FunctionTestName = functionTestName;
}

static void SkipFunctionTest(const string & msg)
{
    cerr << UnitTestName << "[" << FunctionTestName  << "] Skip: " << msg << endl;
    FunctionTestName = "";
}

static void ExitUnitTest()
{
    if (FunctionTestName != "")
    {
        cerr << UnitTestName << "[" << FunctionTestName  << "] Success" << endl;
    }
    cerr << UnitTestName << " : Completed successfully" << endl;
    exit(0);
}

static void Abort(const string & msg)
{
    cerr << UnitTestName << "[" << FunctionTestName  << "] Failure: " << msg << endl;
    exit(1);
}



/*!
 * Return TRUE if YMM registers in specified contexts are identical.
 */
static BOOL CompareYmmState(WINDOWS_EXTCTXT* pContext1, WINDOWS_EXTCTXT* pContext2)
{

    if (memcmp((unsigned char *)(pContext1->GetLegacyContext()) + XmmRegsOffset,
                  (unsigned char *)(pContext2->GetLegacyContext()) + XmmRegsOffset,
                  NumYmmRegs*YmmRegSize) == 0)
    {
        if (memcmp((char *)(reinterpret_cast<char *>(pContext1->GetYmmContext())), reinterpret_cast<char *>(pContext2->GetYmmContext()), NumYmmRegs*YmmRegSize)!=0)
        {
            cerr << "Failure in ymmUpper reg values compare " << endl;
        }
        else
        {
            return true;
        }
    }
    else
    {
        cerr << "Failure in xmm reg values compare " << endl;
    }
    return false;

}



//==========================================================================
// Function test B)
//==========================================================================
/*!
 * The thread start routine.
 */
static DWORD WINAPI ThreadB(void * pContext)
{
#if 0
    SetMyFpContext(0);
    unsigned char *buf1 = new unsigned char[sizeof (FPSAVE)+FPSTATE_ALIGNMENT];
    FPSAVE *fpstateBuf1 = 
        reinterpret_cast<FPSAVE *>
        (( reinterpret_cast<ADDRINTX>(buf1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf1, 0, sizeof (FPSAVE));
   
    Do_Xsave (fpstateBuf1);
    int *ptr = reinterpret_cast<int *>(fpstateBuf1->_ymmUpper), i;
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("in thread ymm%d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }
#endif
    SignalObjectAndWait(StartEvent, AllowStartEvent, INFINITE, FALSE);
    return 0;
}


void SetYmmRegisters(YMMCONTEXT *Ymm, ULONG64 Value) 
{
   {
        M128A * MyYmm = &Ymm->Ymm0;        
        for(int i = 0; i <= 15;i++) {
            MyYmm[i].Low = Value+i;
            MyYmm[i].High = ~(Value+i);
        }        
    }        
}

#define YmmRegSize 16
#define NumYmmRegs 16

/*!
 * The main procedure of the application.
 */
int main(int argc, char *argv[])
{
    BOOL bStatus;
    DWORD dwStatus;
    HANDLE hThread;
    DWORD suspendCount;

    StartEvent      = CreateEvent(0, FALSE, FALSE, 0);
    AllowStartEvent = CreateEvent(0, TRUE, FALSE, 0);
    //SetMyFpContext(0);

    

    

    //============================================================================ 
    // B) Verify that Pin correctly reads/writes YMM state of threads suspended in 
    //    a system call.
    //============================================================================ 
    StartFunctionTest("B");

    // Suspend a thread in a system call and set some predefined XMM state for
    // the suspended thread. Read the context and check the state of XMM registers.
   
        
        

        // Create a thread and wait until it hangs in a system call.
        ResetEvent(AllowStartEvent);
        hThread = CreateThread(0, 0, ThreadB, 0, 0, 0);
        if (hThread == 0) {Abort("CreateThread failed");}
        WaitForSingleObject(StartEvent, INFINITE);

        // Suspend the thread in a system call.
        suspendCount = SuspendThread(hThread);
        if (suspendCount == DWORD(-1)) {Abort("SuspendThread failed");}
        if (suspendCount != 0) {Abort("Unexpected suspend count in SuspendThread");}


        // The "predefined" YMM state
        const size_t xmm_ymmUpperStateSize = YmmRegSize*NumYmmRegs;
        char xmm_ymmUpperState[xmm_ymmUpperStateSize];
        memset(xmm_ymmUpperState, 1, xmm_ymmUpperStateSize);
        WINDOWS_EXTCTXT ctxt, ctxt1;

        // Read the original YMM state
#ifdef TARGET_IA32E
       ctxt.GetLegacyContext()->ContextFlags = CONTEXT_FLOATING_POINT | CONTEXT_XSTATE;
#else
       ctxt.GetLegacyContext()->ContextFlags = CONTEXT_XSTATE | CONTEXT_EXTENDED_REGISTERS; 
#endif
        PCONTEXT_EX extendedContext =  (PCONTEXT_EX)(ctxt.GetLegacyContext()+1);
        PXSTATE xStateInfo = reinterpret_cast<PXSTATE>((reinterpret_cast<ADDRINT>(extendedContext)) + extendedContext->XState.Offset);
        PYMMCONTEXT pymmContext = &xStateInfo->YmmContext;
        
        bStatus = GetThreadContext(hThread, ctxt.GetLegacyContext());
        if (bStatus == 0) {Abort("GetThreadContext#1 failed");}

        
        

        memset(xStateInfo, 0,sizeof(XSTATE));
        xStateInfo->XBV = 0x4; // contains ymm state                
      
        
        

        // Set the "predefined" state for YMM registers
        memcpy((unsigned char *)(ctxt.GetLegacyContext()) + XmmRegsOffset, xmm_ymmUpperState, xmm_ymmUpperStateSize);
        memcpy((unsigned char *)(ctxt.GetYmmContext()), xmm_ymmUpperState, xmm_ymmUpperStateSize);
        
        int * ptr = reinterpret_cast<int *>(pymmContext);
        int i;
        printf ("\nYmm registers in the state to be used for SetThreadContext\n");
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("ymmUpper%d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }
        ptr = reinterpret_cast<int *>((unsigned char *)(ctxt.GetLegacyContext()) + XmmRegsOffset);
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("xmm     %d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }
        
        bStatus = SetThreadContext(hThread, ctxt.GetLegacyContext());
        if (bStatus == 0) {Abort("SetThreadContext failed");}

        // zero out the ymm regs
        memset((unsigned char *)(ctxt.GetLegacyContext()) + XmmRegsOffset, 0, xmm_ymmUpperStateSize);
        memset((unsigned char *)(ctxt.GetYmmContext()), 0, xmm_ymmUpperStateSize);

        ptr = reinterpret_cast<int *>(pymmContext);
        printf ("\nYmm registers zeroed out in the state to be retrieved\n");
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("ymmUpper%d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }
        ptr = reinterpret_cast<int *>((unsigned char *)(ctxt.GetLegacyContext()) + XmmRegsOffset);
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("xmm     %d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }

        // Read and check the state for YMM registers
        bStatus = GetThreadContext(hThread, ctxt.GetLegacyContext());
        if (bStatus == 0) {Abort("GetThreadContext#2 failed");}
        
        
        ptr = reinterpret_cast<int *>(pymmContext);
        printf ("\nYmm registers in the state retrieved\n");
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("ymmUpper%d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }
        ptr = reinterpret_cast<int *>((unsigned char *)(ctxt.GetLegacyContext()) + XmmRegsOffset);
        for (i=0; i<NumYmmRegs; i++)
        {
            printf ("xmm     %d %x %x %x %x\n", i, *(ptr+3), *(ptr+2), *(ptr+1), *(ptr));
            ptr += 4;
        }

        memcpy((unsigned char *)(ctxt1.GetLegacyContext()) + XmmRegsOffset, xmm_ymmUpperState, xmm_ymmUpperStateSize);
        memcpy((unsigned char *)(ctxt1.GetYmmContext()), xmm_ymmUpperState, xmm_ymmUpperStateSize);
        if (!CompareYmmState(&ctxt, &ctxt1))
        {
            Abort("Mismatch in the YMM state");
        }



        // Let the thread exit
        SetEvent(AllowStartEvent);
        suspendCount = ResumeThread(hThread);
        if (suspendCount == DWORD(-1)) {Abort("ResumeThread failed");}
        dwStatus = WaitForSingleObject(hThread, 60000); 
        if (dwStatus != WAIT_OBJECT_0) {Abort("The thread did not exit");}

        CloseHandle(hThread);
   


    //============================================================================ 
    ExitUnitTest();

    return 0;
}
