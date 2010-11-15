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
 * This test checks the status of a system call in a way that is specific to
 * the IA-64 architecture.
 */

#include <iostream>
#include <fstream>
#include <syscall.h>

#include "pin.H"

using namespace std;

ofstream trace;

// Print syscall number and arguments
VOID SysBefore(ADDRINT ip, ADDRINT num, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2,
               ADDRINT arg3, ADDRINT arg4, ADDRINT arg5)
{
    trace << "@ip 0x" << hex << ip << ": sys call " << dec << num;
    trace << "(0x" << hex << arg0 << ", 0x" << arg1 << ", 0x" << arg2;
    trace << hex << ", 0x" << arg3 << ", 0x" << arg4 << ", 0x" << arg5 << ")" << endl;
}

// Print the return value of the system call
VOID SysAfter(ADDRINT value, UINT64 err, UINT64 g8, UINT64 g10 )
{
    int error = 0;
    ADDRINT neg_one = 0-1;
    
    if ( err == 0 ) 
    {
        if ( g8 != value )
            error = 1;
        if ( err != g10 )
            error = 2;
    }
    else
    {
        if ( value != neg_one )
            error = 3;
        if ( err != g8 )
            error = 4;
    }

    if ( error == 0 )
        trace << "Success: value=0x" << hex << value << ", errno=" << dec << err << endl;
    else 
    {
        trace << "Failure " << error << ": value=0x" << hex << value << ", errno=" << dec << err;
        trace << ", g8=0x" << hex << g8 << ", g10=0x" << g10 << endl;
    }
    
    trace << endl;
}


// Is called for every instruction and instruments syscalls
VOID Instruction(INS ins, VOID *v)
{
    if (INS_IsSyscall(ins))
    {
        // Arguments and syscall number is only available before
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SysBefore),
                       IARG_INST_PTR, IARG_SYSCALL_NUMBER,
                       IARG_SYSARG_VALUE, 0, IARG_SYSARG_VALUE, 1,
                       IARG_SYSARG_VALUE, 2, IARG_SYSARG_VALUE, 3,
                       IARG_SYSARG_VALUE, 4, IARG_SYSARG_VALUE, 5,
                       IARG_END);


        // return value only available after
        INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(SysAfter),
                       IARG_SYSRET_VALUE, IARG_SYSRET_ERRNO,
                       IARG_REG_VALUE, REG_G08, IARG_REG_VALUE, REG_G10,
                       IARG_END);

    }
}

VOID Fini(INT32 code, VOID *v)
{
    trace << "#eof" << endl;
    trace.close();
}


int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);

    trace.open( "strace.out" );
    if ( ! trace.is_open() ) 
    {
        cout << "Could not open strace.out" << endl;
        exit(1);
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
