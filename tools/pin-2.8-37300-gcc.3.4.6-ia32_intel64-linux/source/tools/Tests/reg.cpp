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
#include <fstream>
#include <stdlib.h>
#include "pin.H"

ofstream out("reg.out");

UINT64 icount = 0;

VOID docount(VOID * ip, VOID * reg)
{
    icount++;
    if ((icount % 1000) == 1)
        out << "ip:" << ip << " count:" << icount << " SP:" << reg << endl;
}
    
VOID Instruction(INS ins, VOID *v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,
                   IARG_INST_PTR, IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
}

VOID PrintError(ADDRINT ip, ADDRINT al, ADDRINT ah, ADDRINT ax, ADDRINT eax, ADDRINT rax)
{
    std::cerr << hex << "ip " << ip
              << " al " << al
              << " ah " << ah
              << " ax " << ax
              << " eax " << eax
              << " rax " << rax
              << endl;
    exit(1);
}
    
VOID CheckRegs(ADDRINT ip, ADDRINT al, ADDRINT ah, ADDRINT ax, ADDRINT eax, ADDRINT rax)
{
    if ((eax & 0xff) != al)
    {
        PrintError(ip, al, ah, ax, eax, rax);
    }
    
    if (((eax & 0xff00) >> 8) != ah)
    {
        PrintError(ip, al, ah, ax, eax, rax);
    }

    if ((eax & 0xffff) != ax)
    {
        PrintError(ip, al, ah, ax, eax, rax);
    }

#if defined(TARGET_IA32E)
    if((rax & 0xffffffff) != eax)
    {
        PrintError(ip, al, ah, ax, eax, rax);
    }
#endif
}

VOID CheckArRegs(ADDRINT rsc,
                 ADDRINT bsp,
                 ADDRINT bspstore,
                 ADDRINT rnat,
                 ADDRINT fcr,
                 ADDRINT eflag,
                 ADDRINT csd,
                 ADDRINT ssd,
                 ADDRINT cflg,
                 ADDRINT fsr,
                 ADDRINT fir,
                 ADDRINT fdr,
                 ADDRINT ccv,
                 ADDRINT unat,
                 ADDRINT fpsr,
                 ADDRINT itc,
                 ADDRINT pfs,
                 ADDRINT lc,
                 ADDRINT ec)
{}

VOID CheckNat(ADDRINT nat)
{}

ADDRINT capturedVal;

// Make it inlineable
VOID CaptureRef(ADDRINT *ref)
{
    capturedVal = *ref;
}

VOID CheckRef(char const * name, ADDRINT val, ADDRINT *ref, ADDRINT const *const_ref)
{
    if (val != capturedVal
        || val != *ref
        || val != *const_ref)
    {
        std::cerr << hex << name << " val: " << val << " ref: " << *ref << " const_ref: " << *const_ref << endl;
        exit(1);
    }
}

ADDRINT dummy;

VOID InsertCheckRef(TRACE trace, char const * name, REG reg)
{
    // This is inlineable
    TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(CaptureRef),
                     IARG_PTR, &dummy,
                     IARG_END);
    // This is not inlineable because of IARG_REG_REFERENCE
    TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(CaptureRef),
                     IARG_REG_REFERENCE, reg,
                     IARG_END);
    TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(CheckRef),
                     IARG_PTR, name,
                     IARG_REG_VALUE, reg,
                     IARG_REG_REFERENCE, reg,
                     IARG_REG_CONST_REFERENCE, reg,
                     IARG_END);
}
    
VOID Trace(TRACE trace, VOID *v)
{
    static BOOL first = TRUE;
    
#if defined(TARGET_IPF)
    if (first)
    {
        TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(CheckArRegs),
                         IARG_REG_VALUE, REG_AR_RSC,
                         IARG_REG_VALUE, REG_AR_BSP,
                         IARG_REG_VALUE, REG_AR_BSPSTORE,
                         IARG_REG_VALUE, REG_AR_RNAT,

                         IARG_REG_VALUE, REG_AR_FCR,
    
                         IARG_REG_VALUE, REG_AR_EFLAG,
                         IARG_REG_VALUE, REG_AR_CSD,
                         IARG_REG_VALUE, REG_AR_SSD,
                         IARG_REG_VALUE, REG_AR_CFLG,
                         IARG_REG_VALUE, REG_AR_FSR,
                         IARG_REG_VALUE, REG_AR_FIR,
                         IARG_REG_VALUE, REG_AR_FDR,
    
                         IARG_REG_VALUE, REG_AR_CCV,
                         IARG_REG_VALUE, REG_AR_UNAT,

                         IARG_REG_VALUE, REG_AR_FPSR,

                         IARG_REG_VALUE, REG_AR_ITC,

                         IARG_REG_VALUE, REG_AR_PFS,
                         IARG_REG_VALUE, REG_AR_LC,
                         IARG_REG_VALUE, REG_AR_EC,
                         IARG_END);
    }

    TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(CheckNat),
                     IARG_INST_PTR,
                     IARG_REG_NAT_VALUE, REG_G03,
                     IARG_END);
#endif    

    
#if defined(TARGET_IA32) || defined(TARGET_IA32E)

    if (first)
    {
        InsertCheckRef(trace, "gax", REG_GAX);
        InsertCheckRef(trace, "stack", REG_STACK_PTR);
    }

    TRACE_InsertCall(trace, IPOINT_BEFORE, AFUNPTR(CheckRegs),
                     IARG_INST_PTR,
                     IARG_REG_VALUE, REG_AL,
                     IARG_REG_VALUE, REG_AH,
                     IARG_REG_VALUE, REG_AX,
                     IARG_REG_VALUE, REG_EAX,
#if defined(TARGET_IA32E)
                     IARG_REG_VALUE, REG_RAX,
#endif
                     IARG_END
    );

    INS ins = BBL_InsHead(TRACE_BblHead(trace));

    static INT32 origCount = 0;
    if (INS_IsOriginal(ins))
    {
        origCount++;
    }
    
    static INT32 immcount = 10;
    if (immcount > 0)
    {
        for (UINT32 op = 0; op < INS_OperandCount(ins); op++)
        {
            if (INS_OperandIsImmediate(ins, op))
            {
                std::cerr << "Immediate: " << hex << INS_OperandImmediate(ins, op) << " ins " << INS_Disassemble(ins) << endl;
                immcount--;
            }
        }
    }
#endif

    first = FALSE;
}

int main(INT32 argc, CHAR **argv)
{
    PIN_Init(argc, argv);
    
    GetVmLock();
    ReleaseVmLock();
    
    INS_AddInstrumentFunction(Instruction, 0);
    TRACE_AddInstrumentFunction(Trace, 0);
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
