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
 * This test tool verifies correctness of the PIN_RaiseException 
 * API implementation.
 */

#include "pin.H"

#include <string>
#include <iostream>

using namespace std;

extern "C" unsigned int ymmInitVals[];
unsigned int ymmInitVals[64];
extern "C" unsigned int xmmSaveVal[];
unsigned int xmmSaveVal[4];

typedef unsigned __int64 ULONG64;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;


#if defined(TARGET_IA32)

const size_t FpRegSize = 16;
const size_t NumFpRegs = 8;

const size_t XmmRegSize = 16;
const size_t NumXmmRegs = 8;

const size_t YmmRegUpperSize = 16;
const size_t NumYmmRegs = 8;


#elif defined(TARGET_IA32E)

const size_t FpRegSize = 16;
const size_t NumFpRegs = 8;

const size_t XmmRegSize = 16;
const size_t NumXmmRegs = 16;

const size_t YmmRegUpperSize = 16;
const size_t NumYmmRegs = 16;


#endif


extern "C" void Do_Xsave(FPSTATE *xsaveContext);
extern "C" void Do_Fxsave(FPSTATE *xsaveContext);
extern "C" void Do_Xrstor(FPSTATE *xsaveContext);
extern "C" void Do_Fxrstor(FPSTATE *xsaveContext);

/*!
 * Set some constant values in FP and XMM and upper part of YMM registers. 
 */
void SetMyFpContext(int valToAdd)
{
    unsigned char *buf1 = new unsigned char[sizeof (FPSTATE)+FPSTATE_ALIGNMENT];
    FPSTATE *fpstateBuf1 = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINT>(buf1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf1, 0, sizeof (FPSTATE));
   
    Do_Xsave (fpstateBuf1);
    

    
    memset(&(fpstateBuf1->fxsave_legacy._st), 0, sizeof (fpstateBuf1->fxsave_legacy._st));
    for (size_t i = 0; i <NumFpRegs; ++i)
    {
        *((unsigned char *)(&(fpstateBuf1->fxsave_legacy._st)) + (i * FpRegSize)) = (unsigned char)i+valToAdd; 
    }

    memset(&(fpstateBuf1->fxsave_legacy._xmm), 0, sizeof (fpstateBuf1->fxsave_legacy._xmm));
    for (size_t i = 0; i < NumXmmRegs; ++i)
    {
        *((unsigned char *)(&(fpstateBuf1->fxsave_legacy._xmm)) + (i * XmmRegSize)) = (unsigned char)i+valToAdd; 
    }
    

    memset(&(fpstateBuf1->_xstate._ymmUpper), 0, sizeof (fpstateBuf1->_xstate._ymmUpper));
    for (size_t i = 0; i < NumYmmRegs; ++i)
    {
        *((unsigned char *)(&(fpstateBuf1->_xstate._ymmUpper)) + (i * YmmRegUpperSize)) = (unsigned char)i+valToAdd;
    }
    
    fpstateBuf1->_xstate._extendedHeader._mask = 7;
    Do_Xrstor (fpstateBuf1);
    
}


/*!
 * RTN analysis routines.
 */
static VOID OnWriteToNull(const CONTEXT * ctxt, THREADID tid)
{
    printf ("OnWriteToNull\n"); fflush (stdout);    
    SetMyFpContext(1); // set it to different than what application set it
    // Raise EXCEPTION_ACCESS_VIOLATION exception on behalf of the application
    ADDRINT exceptAddr = PIN_GetContextReg(ctxt, REG_INST_PTR);
    EXCEPTION_INFO exceptInfo;
    PIN_InitExceptionInfo(&exceptInfo, EXCEPTCODE_ACCESS_INVALID_ADDRESS, exceptAddr);
    PIN_RaiseException(ctxt, tid, &exceptInfo);
}

CHAR fpContextSpaceForFpConextFromPin[FPSTATE_SIZE+FPSTATE_ALIGNMENT];
FPSTATE *fpContextFromPin; 
    

static VOID OnRaiseX87OverflowException(const CONTEXT * ctxt, THREADID tid)
{
    printf ("OnRaiseX87OverflowException\n"); fflush (stdout);
    FPSTATE *fpContextFromPin = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINT>(fpContextSpaceForFpConextFromPin) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    PIN_GetContextFPState(ctxt, fpContextFromPin);
    SetMyFpContext(1); // set it to different than what application set it
    // Raise FP_OVERFLOW exception on behalf of the application
    ADDRINT exceptAddr = PIN_GetContextReg(ctxt, REG_INST_PTR);
    EXCEPTION_INFO exceptInfo;
    PIN_InitExceptionInfo(&exceptInfo, EXCEPTCODE_X87_OVERFLOW, exceptAddr);
    PIN_RaiseException(ctxt, tid, &exceptInfo);
}

static VOID OnRaiseSystemException(const CONTEXT * ctxt, THREADID tid, ADDRINT sysExceptCode)
{
    printf ("OnRaiseSystemException\n"); fflush (stdout);
    SetMyFpContext(1); // set it to different than what application set it
    // Raise FP_OVERFLOW exception on behalf of the application
    ADDRINT exceptAddr = PIN_GetContextReg(ctxt, REG_INST_PTR);
    EXCEPTION_INFO exceptInfo;
    PIN_InitWindowsExceptionInfo(&exceptInfo, static_cast<UINT32>(sysExceptCode), exceptAddr);
    PIN_RaiseException(ctxt, tid, &exceptInfo);
}

/*!
 * RTN instrumentation routine.
 */
static VOID InstrumentRoutine(RTN rtn, VOID *)
{
    if (RTN_Name(rtn) == "WriteToNull")
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(OnWriteToNull), 
            IARG_CONTEXT,
            IARG_THREAD_ID,
            IARG_END);
        RTN_Close(rtn);
    }
    if (RTN_Name(rtn) == "RaiseX87OverflowException")
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(OnRaiseX87OverflowException), 
            IARG_CONTEXT,
            IARG_THREAD_ID,
            IARG_END);
        RTN_Close(rtn);
    }
    else if (RTN_Name(rtn) == "RaiseSystemException")
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(OnRaiseSystemException), 
            IARG_CONTEXT,
            IARG_THREAD_ID,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
        RTN_Close(rtn);
    }
}

VOID Fini(INT32 code, VOID *v)
{
}

/*!
 * The main procedure of the tool.
 */
int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    RTN_AddInstrumentFunction(InstrumentRoutine, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
