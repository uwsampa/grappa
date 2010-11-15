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
#include <fstream>
#include <iostream>
#include "pin.H"

KNOB<BOOL> KnobWaitForDebugger(KNOB_MODE_WRITEONCE, "pintool",
    "wait_for_debugger", "0", "Wait for debugger to connect");
KNOB<std::string> KnobOut(KNOB_MODE_WRITEONCE, "pintool",
    "o", "", "Output file where TCP information is written");


BOOL IsFirstIns = TRUE;
BOOL IsFirstBreakpoint = TRUE;

static void InstrumentIns(INS, VOID *);
static void DoBreakpoint(CONTEXT *, THREADID);


int main(int argc, char * argv[])
{
    PIN_Init(argc, argv);

    if (!KnobOut.Value().empty())
    {
        DEBUG_CONNECTION_INFO info;
        PIN_GetDebugConnectionInfo(&info);

        std::ofstream out(KnobOut.Value().c_str());
        out << std::dec << info._tcpServer._tcpPort;
    }

    INS_AddInstrumentFunction(InstrumentIns, 0);

    PIN_StartProgram();
    return 0;
}


static void InstrumentIns(INS ins, VOID *)
{
    if (IsFirstIns)
    {
        IsFirstIns = FALSE;
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(DoBreakpoint), IARG_CONTEXT, IARG_THREAD_ID, IARG_END);
    }
}

static void DoBreakpoint(CONTEXT *ctxt, THREADID tid)
{
    if (IsFirstBreakpoint)
    {
        IsFirstBreakpoint = FALSE;
        PIN_ApplicationBreakpoint(ctxt, tid, KnobWaitForDebugger.Value(), "The tool wants to stop");
    }
}
