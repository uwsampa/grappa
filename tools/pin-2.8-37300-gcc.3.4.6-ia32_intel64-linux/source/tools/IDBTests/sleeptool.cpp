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
/*
 * This tool is part of the "sleep" test, which attempts to single-step over a system call
 * trap.  It is difficult to construct this test because we don't know how many machine
 * instructions the debugger needs to single-step over.  To solve this, we use a Pin tool
 * to count the instruction.  This tool does that and generates and IDB command file with
 * the right number of single-step commands.
 */

#include <fstream>
#include "pin.H"

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "sleeptool.out", "IDB command file");


static VOID InstrumentImage(IMG, VOID *);
static VOID InstrumentIns(INS, VOID *);
static VOID AtBefore();
static VOID AtAfter();
static VOID DoCount();
static VOID OnExit(INT32, VOID *);


static BOOL EnableCount = FALSE;
static UINT64 Count = 0;


int main(int argc, char * argv[])
{
    PIN_Init(argc, argv);
    PIN_InitSymbols();

    IMG_AddInstrumentFunction(InstrumentImage, 0);
    INS_AddInstrumentFunction(InstrumentIns, 0);
    PIN_AddFiniFunction(OnExit, 0);

    PIN_StartProgram();
    return 0;
}


static VOID InstrumentImage(IMG img, VOID *)
{
    RTN rtn = RTN_FindByName(img, "BeforeSleep");
    if (RTN_Valid(rtn))
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(AtBefore), IARG_END);
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "AfterSleep");
    if (RTN_Valid(rtn))
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(AtAfter), IARG_END);
        RTN_Close(rtn);
    }
}

static VOID InstrumentIns(INS ins, VOID *)
{
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(DoCount), IARG_END);
}

static VOID AtBefore()
{
    EnableCount = TRUE;
}

static VOID AtAfter()
{
    EnableCount = FALSE;
}

static VOID DoCount()
{
    if (EnableCount)
        Count++;
}

static VOID OnExit(INT32, VOID *)
{
    std::ofstream out(KnobOutputFile.Value().c_str());

    out << "break BeforeSleep\n";
    out << "run\n";
    out << "stepi " << Count << "\n";
    out << "kill\n";
    out << "quit\n";
}
