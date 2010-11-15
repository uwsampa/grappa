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
/* ===================================================================== */
/*
  @ORIGINAL_AUTHOR: Robert Muth
*/

/* ===================================================================== */
/*! @file
 *  This file contains an ISA-portable PIN tool for counting number of returns
 *  This primarily tests inlined analysis of return instructions.
 */

#include "pin.H"
#include <iostream>

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

UINT64 num_rets = 0;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool tests inlined analysis of return instructions.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

ADDRINT AnanlRet_if(UINT32 executing, unsigned long landing_pc, unsigned long ret, 
                        unsigned long sp, size_t tid)
{
    // Just do some useless computation
    unsigned long t = landing_pc >> 4;
    t = (t << 4) & 0;
    return executing;
}

void fakeCallFunction(unsigned long ret, unsigned long sp, size_t tid)
{
    // more useless computation
    unsigned long t = sp >> 4;
    t = (t << 4) & 0;
    return;
}
    
void AnanlRet_then(unsigned long landing_pc, unsigned long ret)
{
    // Do more useless computation and make sure you call some other function
    unsigned long t = landing_pc >> 4;
    t = (t << 4) & 0;
    fakeCallFunction(ret, 200, 0);
    t = ret >> 4;
    t = (t << 4) & 0;
    return;
}

/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v)
{
  for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) ) {
    INS head = BBL_InsHead(bbl);
    INS tail = BBL_InsTail(bbl);
    if( INS_IsRet(tail) ) {
            INS_InsertIfCall(tail, IPOINT_TAKEN_BRANCH,
                                   (AFUNPTR)AnanlRet_if,
                                   IARG_EXECUTING,
                                   IARG_BRANCH_TARGET_ADDR,
                                   IARG_G_RESULT0,
                                   IARG_REG_VALUE, REG_STACK_PTR,
                                   IARG_REG_VALUE, REG_THREAD_ID,
                                   IARG_END);
            INS_InsertThenCall(tail, IPOINT_TAKEN_BRANCH,
                                     (AFUNPTR)AnanlRet_then,
                                     IARG_BRANCH_TARGET_ADDR,
                                     IARG_G_RESULT0,
                                     IARG_END);
            num_rets++;
    }
  }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    cerr <<  "num_rets : " << num_rets  << endl;
    
}

/* ===================================================================== */

int main(int argc, char *argv[])
{
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    

  TRACE_AddInstrumentFunction(Trace, 0);
  PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
