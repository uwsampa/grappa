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
 *  A pin tool that intercepts exception and verifies the exception context.
 *  It works in pair with the winapp_exception_context application, that raises two 
 *  exceptions. The context of the second exception should have a predefined FP 
 *  state: all FPn and XMMn and YMMn registers have value <n> in their first byte. 
 *  This is verified by the tool.
 */

#include <stdio.h>
#include <string>
#include <iostream>
#include <memory.h>
#include "pin.H"

using namespace std;


/*
 * Verify the FP state of the exception context
 */
KNOB<BOOL> KnobCheckFp(KNOB_MODE_WRITEONCE, "pintool", "checkfp", "1", "Check FP state");




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
 * Exit with the specified error message
 */
static void Abort(const char * msg)
{
    cerr << msg << endl;
    exit(1);
}

/*!
 * Check to see that FP/XMM registers in the specified context have predefined
 * values assigned by the application: FPn and XMMn registers have value <n> in 
 * their first byte. 
 */
static bool CheckFpContextPinContext(const CONTEXT * pContext, int numToAdd)
{
	unsigned char fpContextSpaceForXsave[sizeof (FPSTATE)+ FPSTATE_ALIGNMENT];
    FPSTATE *fpState= 
		reinterpret_cast<FPSTATE *>
		(( reinterpret_cast<ADDRINT>(fpContextSpaceForXsave) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));

    PIN_GetContextFPState(pContext, fpState);

    for (size_t i = 0; i < sizeof(fpState->fxsave_legacy._st) ; ++i)
    {
        UINT8 regId = i/16 + numToAdd;
        if ((i%16 == 0) && (fpState->fxsave_legacy._st[i] != regId))
        {
            return false;
        }
        if ((i%16 != 0) && (fpState->fxsave_legacy._st[i] != 0))
        {
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(fpState->fxsave_legacy._xmm); ++i)
    {
        UINT8 regId = i/16 + numToAdd;
        if ((i%16 == 0) && (fpState->fxsave_legacy._xmm[i] != regId))
        {
            return false;
        }
        if ((i%16 != 0) && (fpState->fxsave_legacy._xmm[i] != 0))
        {
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(fpState->_xstate._ymmUpper); ++i)
    {
        UINT8 regId = i/16 + numToAdd;
        if ((i%16 == 0) && (fpState->_xstate._ymmUpper[i] != regId))
        {
            return false;
        }
        if ((i%16 != 0) && (fpState->_xstate._ymmUpper[i] != 0))
        {
            return false;
        }
    }

    return true;
}

static void OnException(THREADID threadIndex, 
                  CONTEXT_CHANGE_REASON reason, 
                  const CONTEXT *ctxtFrom,
                  CONTEXT *ctxtTo,
                  INT32 info, 
                  VOID *v)
{
    if (reason == CONTEXT_CHANGE_REASON_EXCEPTION)
    { 
        {
            if (KnobCheckFp && !CheckFpContextPinContext(ctxtFrom,0))
            {
                Abort("Tool: Mismatch in the FP context");
            }
			SetMyFpContext(1);
        }
    }
}


int main(INT32 argc, CHAR **argv)
{
    if (PIN_Init(argc, argv))
    {
        Abort("Tool: Invalid arguments");
    }

    PIN_AddContextChangeFunction(OnException, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
