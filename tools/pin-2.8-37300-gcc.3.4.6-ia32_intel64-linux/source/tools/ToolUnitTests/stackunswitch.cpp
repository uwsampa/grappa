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
#include <iostream>
#include "pin.H"


#ifndef ASM
extern "C" VOID* StackUnswitch(AFUNPTR fun, VOID* arg1, VOID* arg2, VOID* arg3, VOID* arg4, ADDRINT appSp);
#endif
    
ADDRINT foo(ADDRINT x1, ADDRINT x2, ADDRINT x3, ADDRINT x4)
{
    static BOOL first = TRUE;

    if (first)
    {
        first = FALSE;
        //cout << "In foo(): x1="<<decstr(x1)<<" x2="<<decstr(x2)<<" x3="<<decstr(x3)<<" x4="<<decstr(x4)<<endl;
    }
    
    return x1+x2+x3+x4;
}

ADDRINT bar(ADDRINT x1, ADDRINT x2)
{
    static BOOL first = TRUE;

    if (first)
    {
        first = FALSE;
        //cout << "In bar(): x1="<<decstr(x1)<<" x2="<<decstr(x2)<<endl;
    }
    
    return x1*x2;
}


GLOBALCFUN VOID* MyDispatch(AFUNPTR fun, VOID* arg1, VOID* arg2, VOID* arg3, VOID* arg4)
{
    if (fun == reinterpret_cast<AFUNPTR>(foo))
    {  // call foo()
        return reinterpret_cast<VOID*>(foo(reinterpret_cast<ADDRINT>(arg1),
                                           reinterpret_cast<ADDRINT>(arg2),
                                           reinterpret_cast<ADDRINT>(arg3),
                                           reinterpret_cast<ADDRINT>(arg4)));
    }
    else if (fun == reinterpret_cast<AFUNPTR>(bar))
    {  // call bar()
        return reinterpret_cast<VOID*>(bar(reinterpret_cast<ADDRINT>(arg1),
                                           reinterpret_cast<ADDRINT>(arg2)));
    } 
    ASSERT(false,"MyDistpatch(): unknown function\n");
    
    return 0;
}

VOID DoBbl(THREADID threadId)
{

    const ADDRINT appSp = PIN_FindAlternateAppStack();

    if (threadId == 1)
    {
        reinterpret_cast<ADDRINT>(StackUnswitch(reinterpret_cast<AFUNPTR>(foo),
                                                reinterpret_cast<VOID*>(10), reinterpret_cast<VOID*>(20),
                                                reinterpret_cast<VOID*>(30), reinterpret_cast<VOID*>(40),
                                                appSp));
    }
    else if (threadId == 2)
    {
        reinterpret_cast<ADDRINT>(StackUnswitch(reinterpret_cast<AFUNPTR>(bar),
                                                reinterpret_cast<VOID*>(100), reinterpret_cast<VOID*>(200),
                                                reinterpret_cast<VOID*>(0), reinterpret_cast<VOID*>(0),
                                                appSp));
    }
    
        
    //cout << "Result of calling foo = "<<decstr(result)<<endl;
    
}

// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID *v)
{
    // Visit every basic block in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
         BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)DoBbl, IARG_THREAD_ID, IARG_END);
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    // Initialize pin
    PIN_Init(argc, argv);

    // Register Instruction to be called to instrument instructions
    TRACE_AddInstrumentFunction(Trace, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
