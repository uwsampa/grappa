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
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include "pin.H"

map<ADDRINT,INT32> divisors;

// Value profile for a div instruction
class DIVPROF
{
  public:
    DIVPROF(ADDRINT trace, ADDRINT instruction) : _traceAddress(trace), _instructionAddress(instruction)
    {
        _count = 0;
        memset(_divisors, 0, sizeof(_divisors[0]) * NumDiv);
    };
    static VOID ProfileDivide(DIVPROF * dp, INT32 divisor);
    
  private:
    enum 
    {
        NumDiv = 8,
        OptimizeThreshold = 50
    };

    INT32 CommonDivisor();
    VOID InsertProfile(INS ins);
    
    const ADDRINT _traceAddress;
    const ADDRINT _instructionAddress;
    INT32 _count;
    INT32 _divisors[NumDiv];
};

INT32 DIVPROF::CommonDivisor()
{
    for (INT32 i = 1; i < NumDiv; i++)
    {
        if (_divisors[i] * 2 > _count)
        {
            return i;
        }
    }

    return 0;
}
    
VOID DIVPROF::ProfileDivide(DIVPROF * dp, INT32 divisor)
{
    // Profile the divisors in the interesting range
    if (divisor >= 1 && divisor <= NumDiv)
        dp->_divisors[divisor]++;

    // If we exceeded the execution threshold, decide what to do with this divisor
    dp->_count++;
    if (dp->_count < OptimizeThreshold)
        return;
    
    std::cout << "Instruction exceeded threshold at " << hex << dp->_instructionAddress << endl;

    // If we haven't already made a decision, pick a likely divisor
    // 0 means no divisor
    if (divisors.find(dp->_instructionAddress) == divisors.end())
        divisors[dp->_instructionAddress] = dp->CommonDivisor();

    // Unlink this trace so we will remove the instrumentation and optimize it
    CODECACHE_InvalidateTraceAtProgramAddress(dp->_traceAddress);
}

LOCALFUN VOID InsertProfile(TRACE trace, INS ins)
{
    DIVPROF * dp = new DIVPROF(TRACE_Address(trace), INS_Address(ins));
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(DIVPROF::ProfileDivide),
                   IARG_PTR, dp, IARG_REG_VALUE, INS_OperandReg(ins, 0), IARG_END);
}
        
ADDRINT CheckOne(ADDRINT divisor)
{
    return divisor != 1;
}

ADDRINT Div64()
{
    return 0;
}

LOCALFUN VOID OptimizeDivide(INS ins, INT32 divisor)
{
    if (divisor == 1)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Zero), IARG_RETURN_REGS, REG_GDX, IARG_END);
        INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckOne), IARG_REG_VALUE, INS_OperandReg(ins, 0), IARG_END);
        INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(Zero), IARG_RETURN_REGS, REG_GDX, IARG_END);
        INS_Delete(ins);
    }
    
}

VOID Trace(TRACE trace, VOID *v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            if ((INS_Opcode(ins) == XED_ICLASS_IDIV
                 || INS_Opcode(ins) == XED_ICLASS_DIV)
                && INS_OperandIsReg(ins, 0))
            {
                map<ADDRINT,INT32>::const_iterator di = divisors.find(INS_Address(ins));

                if (di == divisors.end())
                {
                    // No information, let's profile it
                    std::cout << "Profiling instruction at " << hex << INS_Address(ins) << endl;
                    InsertProfile(trace, ins);
                }
                else if (di->second > 0)
                {
                    // We found a divisor
                    std::cout << "Optimizing instruction at " << hex << INS_Address(ins) << " with value " << di->second << endl;
                    OptimizeDivide(ins, di->second);
                }
                else
                {
                    std::cout << "no optimizing or profile of instruction at " << hex << INS_Address(ins) << " with value " << di->second << endl;
                }

            }
        }
    }
}
            

int main(int argc, char * argv[])
{
    PIN_Init(argc, argv);

    TRACE_AddInstrumentFunction(Trace, 0);
    
    PIN_StartProgram();
    
    return 0;
}
