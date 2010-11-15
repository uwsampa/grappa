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
// Pin tool that interacts with the profiler by passing the object address

#include <stdio.h>
#include "pin.H"
#include <typeinfo>

// include interface to profile
#include "insprofiler.H"

// pointer to profiler object - will be constructed in main
Profiler *prof;

VOID Analysis(VOID *val)
{
    prof = (Profiler *)(val);
    prof->Hit();
}

VOID Instruction(INS ins, VOID *val)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) Analysis, IARG_PTR, val, IARG_END);
}

VOID Fini(INT32 code, VOID *val)
{
    prof = (Profiler *)(val);
    prof->Stats();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool interacts with the profiler by passing the object address" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    // Create a Profiler Object
    Profiler *prof = new Profiler();

    // Passing profiler object to the instrumentation function
    INS_AddInstrumentFunction(Instruction, prof);

    // Passing profiler object to the finish function
    PIN_AddFiniFunction(Fini, prof);
    
    PIN_StartProgram();
    
    return 0;
}
