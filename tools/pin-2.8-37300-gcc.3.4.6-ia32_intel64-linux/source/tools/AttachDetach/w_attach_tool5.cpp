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
 * This test tool instruments every instruction of the sw_interrupt_app program
 * to check correctness of the context recovery for software interrupts.
 */

#include "pin.H"
#include <string>
#include <iostream>

using namespace std;

/* ===================================================================== */
/* Global variables and declarations */
/* ===================================================================== */
typedef int (__cdecl * DO_LOOP_TYPE)();

static volatile int doLoopPred = 1;

/* ===================================================================== */

int rep_DoLoop()
{
    PIN_LockClient();
        
    volatile int localPred =  doLoopPred;
    
    PIN_UnlockClient(); 
    
    return localPred;
}

/*!
 * Context change callback.
 */
static void OnContextChange(THREADID threadIndex, 
                  CONTEXT_CHANGE_REASON reason, 
                  const CONTEXT *ctxtFrom,
                  CONTEXT *ctxtTo,
                  INT32 info, 
                  VOID *v)
{
    PIN_LockClient();
    static volatile INT32 contextChnageCount = 0;
    contextChnageCount++;
    if (reason == CONTEXT_CHANGE_REASON_EXCEPTION)
    {
#if 0
        UINT32 exceptionCode = info;
        ADDRINT exceptAddr = PIN_GetContextReg(ctxtFrom, REG_INST_PTR);
        cerr << "CONTEXT_CHANGE_REASON_EXCEPTION: " << 
            "Exception code " << hex << exceptionCode << "." <<
            "Context IP " << hex << exceptAddr << "." <<
            endl; 
#endif
    }
    if(contextChnageCount == 20)
    {
        doLoopPred = 0;
    }
    PIN_UnlockClient();
}

VOID ImageLoad(IMG img, VOID *v)
{
    cout << IMG_Name(img) << endl;    
    
    if ( ! IMG_IsMainExecutable(img) )
    {
        return;
    }
    const string sFuncName("DoLoop");
    
    for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
    { 
        string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
        if (undFuncName == sFuncName)
        {
            RTN rtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
            if (RTN_Valid(rtn))
            {
                //eventhough this is not an error - print to cerr (in order to see it on the screen)
                cerr << "Replacing DoLoop() in " << IMG_Name(img) << endl;

                RTN_Replace(rtn, AFUNPTR(rep_DoLoop));
            }           
        }      
    }
}

/*!
 * The main procedure of the tool.
 */
int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_AddContextChangeFunction(OnContextChange, 0);

    PIN_StartProgram();
    return 0;
}
