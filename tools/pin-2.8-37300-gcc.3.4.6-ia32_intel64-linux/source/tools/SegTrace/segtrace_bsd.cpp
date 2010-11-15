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
//
// @ORIGINAL_AUTHOR: Greg Lueck
//

/*! @file
 *
 * This file contains a tool that traces all uses and changes to the FS and GS
 * segments, which are typically used to implement thread-local storage on x86
 * platforms.  
 * The Linux tool was modified to FreeBSD
 */

#include "pin.H"
#include <iostream>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <map>

#if defined(TARGET_BSD)
#   include <sys/syscall.h>
#   include <machine/sysarch.h>
#endif

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "segtracebsd.out", "trace file");

std::ofstream Out;

static VOID Instruction(INS, VOID *);
static BOOL WritesSegment(INS, REG *);
static VOID OnSegReference(UINT32, ADDRINT, ADDRINT, THREADID, ADDRINT);
static VOID OnSegWrite(UINT32, ADDRINT, ADDRINT, THREADID);

static std::string Header(THREADID, ADDRINT);
static std::string SegName(REG);
static VOID SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
static VOID SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);

#if defined(TARGET_BSD)
static std::string SysArchFunc(ADDRINT fun);
#endif

int main(int argc, char * argv[])
{
    PIN_Init(argc, argv);

    Out.open(KnobOutputFile.Value().c_str());

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddSyscallEntryFunction(SyscallEntry, 0);
    PIN_AddSyscallExitFunction(SyscallExit, 0);

    PIN_StartProgram();
    return 0;
}


static VOID Instruction(INS ins, VOID *v)
{
    IARG_TYPE ea;

    if (INS_SegmentPrefix(ins))
    {
        if (INS_IsMemoryRead(ins))
            ea = IARG_MEMORYREAD_EA;
        else
            ea = IARG_MEMORYWRITE_EA;

        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(OnSegReference), IARG_UINT32, INS_SegmentRegPrefix(ins),
            IARG_REG_VALUE, INS_SegmentRegPrefix(ins), IARG_INST_PTR, IARG_THREAD_ID, ea, IARG_END);
    }

    REG seg;
    if (WritesSegment(ins, &seg))
    {
        INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(OnSegWrite), IARG_UINT32, seg, IARG_REG_VALUE, seg,
            IARG_INST_PTR, IARG_THREAD_ID, IARG_END);
    }
}

static BOOL WritesSegment(INS ins, REG *seg)
{
    if (INS_RegWContain(ins, REG_SEG_FS))
    {
        *seg = REG_SEG_FS;
        return TRUE;
    }
    if (INS_RegWContain(ins, REG_SEG_GS))
    {
        *seg = REG_SEG_GS;
        return TRUE;
    }
    if (INS_RegWContain(ins, REG_SEG_ES))
    {
        *seg = REG_SEG_ES;
        return TRUE;
    }
    if (INS_RegWContain(ins, REG_SEG_CS))
    {
        *seg = REG_SEG_CS;
        return TRUE;
    }
    if (INS_RegWContain(ins, REG_SEG_DS))
    {
        *seg = REG_SEG_DS;
        return TRUE;
    }
    if (INS_RegWContain(ins, REG_SEG_SS))
    {
        *seg = REG_SEG_SS;
        return TRUE;
    }
    return FALSE;
}

static VOID OnSegReference(UINT32 ireg, ADDRINT val, ADDRINT pc, THREADID tid, ADDRINT ea)
{
    REG reg = static_cast<REG>(ireg);
    Out << Header(tid, pc) << "reference via " << SegName(reg) << " address: 0x" << std::hex << ea << std::endl;
}

static VOID OnSegWrite(UINT32 ireg, ADDRINT val, ADDRINT pc, THREADID tid)
{
    REG reg = static_cast<REG>(ireg);
    Out << Header(tid, pc) << "modify " << SegName(reg) << std::endl;
}

static VOID SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    if (PIN_GetSyscallNumber(ctxt, std) != SYS_sysarch)
        return;

    ADDRINT pc = PIN_GetContextReg(ctxt, REG_INST_PTR);
    ADDRINT op = PIN_GetSyscallArgument(ctxt, std, 0);
    ADDRINT addr = PIN_GetSyscallArgument(ctxt, std, 1);
    ADDRINT value = 0;

    if (op == AMD64_SET_FSBASE || op == AMD64_SET_GSBASE)
    {
        if (PIN_SafeCopy(&value, Addrint2VoidStar(addr), sizeof(ADDRINT)) != sizeof(ADDRINT))
        {
            Out << Header(threadIndex, pc) << "Failed to read actual TLS pointer" << endl;
        }
    }
    else
    {
        // Remember the location where to write the segment register in REG_INST_G0
        PIN_SetContextReg(ctxt, REG_INST_G0, addr);
        value = addr;
    }

    Out << Header(threadIndex, pc) << "sysarch(" << SysArchFunc(op) << ", 0x" << std::hex << value << ")" << std::endl;
}

static VOID SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    ADDRINT addr = PIN_GetContextReg(ctxt, REG_INST_G0);
    if (!addr)
        return;

    // Reset REG_INST_G0
    PIN_SetContextReg(ctxt, REG_INST_G0, 0);

    ADDRINT pc = PIN_GetContextReg(ctxt, REG_INST_PTR);
    ADDRINT ret = PIN_GetSyscallReturn(ctxt, std);
    ADDRINT value = 0;
    if (ret == (ADDRINT)-1 || PIN_SafeCopy(&value, Addrint2VoidStar(addr), sizeof(ADDRINT)) != sizeof(ADDRINT))
    {
        Out << Header(threadIndex, pc) << "Failed to read actual TLS pointer" << endl;
    }
    Out << Header(threadIndex, pc) << "sysarch returned: " << ", 0x" << std::hex << value << "" << std::endl;
}

static std::string Header(THREADID tid, ADDRINT pc)
{
    std::ostringstream s;
    s << "tid " << std::dec << tid << ", pc 0x" << std::hex << pc << ": ";
    return s.str();
}

static std::string SegName(REG reg)
{
    switch (reg)
    {
      case REG_SEG_FS:
        return "FS";
      case REG_SEG_GS:
        return "GS";
      case REG_SEG_ES:
        return "ES";
      case REG_SEG_CS:
        return "CS";
      case REG_SEG_DS:
        return "DS";
      case REG_SEG_SS:
        return "SS";
      default:
        return "OTHER";
    }
}

#if defined(TARGET_BSD)
static std::string SysArchFunc(ADDRINT fun)
{
    switch (fun)
    {
      case AMD64_SET_GSBASE:
        return "AMD64_SET_GSBASE";
      case AMD64_SET_FSBASE:
        return "AMD64_SET_FSBASE";
      case AMD64_GET_FSBASE:
        return "AMD64_GET_FSBASE";
      case AMD64_GET_GSBASE:
        return "AMD64_GET_GSBASE";
      default:
      {
        std::ostringstream s;
        s << "Unknown: 0x" << std::hex << fun;
        return s.str();
      }
    }
}
#endif
