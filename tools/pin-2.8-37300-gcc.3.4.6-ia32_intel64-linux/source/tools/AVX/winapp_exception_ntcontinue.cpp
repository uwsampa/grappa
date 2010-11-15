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
 *  An example of Windows application that raises exception and verifies
 *  the FP/XMM state. 
 */
#include <windows.h>
#include <string>
#include <iostream>
#include <memory.h>
#include "ntdll.h"
#include "context_ex.h"



using namespace std;

typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;

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



#include "xsave_type.h"
#include "fp_context_manip.h"

#include "ymm_apps_stuff.h"


/*!
 * Exit with the specified error message
 */
static void Abort(const char * msg)
{
    cerr << msg << endl;
    exit(1);
}

unsigned bufForExceptionReturn[sizeof (FPSTATE)+FPSTATE_ALIGNMENT];
FPSTATE *fpstateBufForExceptionReturn;
FXSAVE *fxSaveForExceptionReturn;
WINDOWS_EXTCTXT winExtCtxtForExceptionReturn;
char stackArea[2048];
void MyExceptionReturn ()
{
    // check that state is indeed as expected
    Do_Xsave (fpstateBufForExceptionReturn);
    memcpy (winExtCtxtForExceptionReturn.GetYmmContext(), fpstateBufForExceptionReturn->_ymmUpper, sizeof (YMMCONTEXT));
    memcpy (fxSaveForExceptionReturn, &(fpstateBufForExceptionReturn->fxsave_legacy), sizeof (FXSAVE));
    if (!CheckMyFpContextInContext(winExtCtxtForExceptionReturn.GetLegacyContext(), 1))
    {
        exit (-1);
    }
    exit (0);
}



/*!
 * Exception filter. 
 */
static int enteredFilter = 0;
WINDOWS_EXTCTXT winExtCtxt;
unsigned char buf1 [sizeof (FPSTATE)+FPSTATE_ALIGNMENT];
unsigned char buf2 [sizeof (FPSTATE)+FPSTATE_ALIGNMENT];
static int MyExceptionFilter(LPEXCEPTION_POINTERS exceptPtr)
{
    enteredFilter = 1;
    if (!CheckMyFpContextInContext(exceptPtr->ContextRecord, 0))
    {
        Abort("Mismatch in the FP context");
    }
    
    *(winExtCtxt.GetLegacyContext()) = *(exceptPtr->ContextRecord);
    if (winExtCtxt.HasContextFlags(CONTEXT_XSTATE))
    {
        PCONTEXT_EX extendedContext =  (PCONTEXT_EX)((exceptPtr->ContextRecord)+1);
        PXSTATE xStateInfo = reinterpret_cast<PXSTATE>((reinterpret_cast<ADDRINT>(extendedContext)) + extendedContext->XState.Offset);
        winExtCtxt.m_extContext.buffer = *xStateInfo;
        printf ("extendedContext->XState.Offset %x extendedContext->XState.Length %x\n", extendedContext->XState.Offset, extendedContext->XState.Length);
    }
    FXSAVE *fxSave1;
#ifdef TARGET_IA32
    winExtCtxt.GetLegacyContext()->Eip = reinterpret_cast<ADDRINT>(MyExceptionReturn);
    fxSave1 = reinterpret_cast<FXSAVE *>(&(winExtCtxt.GetLegacyContext()->ExtendedRegisters));
    winExtCtxt.SetContextFlags(CONTEXT_FULL | CONTEXT_EXTENDED_REGISTERS | CONTEXT_XSTATE);
#else
    winExtCtxt.GetLegacyContext()->Rip = reinterpret_cast<ADDRINT>(MyExceptionReturn);
    fxSave1 = reinterpret_cast<FXSAVE *>(&(winExtCtxt.GetLegacyContext()->FltSave));
    winExtCtxt.SetContextFlags(CONTEXT_FULL |  CONTEXT_XSTATE);
#endif
    

    winExtCtxt.GetLegacyContext()->ContextFlags |= CONTEXT_XSTATE;

    unsigned char *buf1 = new unsigned char[sizeof (FPSTATE)+FPSTATE_ALIGNMENT];
    FPSTATE *fpstateBuf1 = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINTX>(buf1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf1, 0, sizeof (FPSTATE));

    SetMyFpContext (1); 
    Do_Xsave (fpstateBuf1);
    memcpy (winExtCtxt.GetYmmContext(), fpstateBuf1->_ymmUpper, sizeof (YMMCONTEXT));
    memcpy (fxSave1, &(fpstateBuf1->fxsave_legacy), sizeof (FXSAVE));
    
    fpstateBufForExceptionReturn = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINTX>(buf2) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
#ifdef TARGET_IA32
    fxSaveForExceptionReturn = reinterpret_cast<FXSAVE *>(&(winExtCtxtForExceptionReturn.GetLegacyContext()->ExtendedRegisters));
#else
    fxSaveForExceptionReturn = reinterpret_cast<FXSAVE *>(&(winExtCtxtForExceptionReturn.GetLegacyContext()->FltSave));
#endif
    winExtCtxtForExceptionReturn.GetLegacyContext()->ContextFlags |= CONTEXT_XSTATE;
    memset(fpstateBufForExceptionReturn, 0, sizeof (FPSTATE));

    SetMyFpContext (2);
    NtContinue (winExtCtxt.GetLegacyContext(), FALSE); // should continue execution at MyExceptionReturn with the state set to winExtCtxt
    exit (-1);

}




/*!
 * The main procedure of the application.
 */
int main(int argc, char *argv[])
{
     
    

    __try
    {
        char * p = 0;
        p++;
        SetMyFpContext(0);
        *p = 0;
    }
    __except (MyExceptionFilter(GetExceptionInformation()))
    {
    }
    
    exit (-1);
}
