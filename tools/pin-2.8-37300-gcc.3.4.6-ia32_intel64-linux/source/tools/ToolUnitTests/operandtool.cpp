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
 * Check some of the static properties of operands.
 *
 * This tool inserts no analysis routines, since it is a check for instruction static properties.
 *
 * Since it needs to know the semantics of machine instructions, it's architecture specific.
 */

#include <fstream>
#include <iostream>
#include <iomanip>

#include <string.h>
#include <stdlib.h>

#include "pin.H"

KNOB<string> KnobOutput(KNOB_MODE_WRITEONCE,"pintool", "o", "operandtool.out", "Name for log file");

static ofstream out;

/* We have this set of operand property functions
    INS_OperandIsAddressGenerator 
    INS_OperandIsBranchDisplacement 
    INS_OperandIsFixedMemop
    INS_OperandIsGsOrFsReg 
    INS_OperandIsImmediate 
    INS_OperandIsImplicit 
    INS_OperandIsMemory 
    INS_OperandIsReg 
    INS_OperandRead 
    INS_OperandReadAndWritten (also mis-spelled INS_OperandReadAndWriten)
    INS_OperandReadOnly 
    INS_OperandWritten 
    INS_OperandWrittenOnly 
    
    We test all of them.
*/

/* Define some instruction properties */
enum operandProp_e
{
    isAddressGenerator = 1<<0,
    isBranchDisplacement = 1<<1,
    isFixedMemoOp = 1<<2,
    isImmediate   = 1<<3,
    isImplicit    = 1<<4,
    isMemory      = 1<<5,
    isReg         = 1<<6,
    isRead        = 1<<7,
    isReadWrite   = 1<<8,
    isReadOnly    = 1<<9,
    isWrite       = 1<<10,
    isWriteOnly   = 1<<11,
    allTests      = (1<<12)-1
};

typedef BOOL (*OperandTestFunction_t)(INS, UINT32);

/* Functions in the same order as above... */
static OperandTestFunction_t testFunctions[] = {
    INS_OperandIsAddressGenerator,
    INS_OperandIsBranchDisplacement,
    INS_OperandIsFixedMemop,
    INS_OperandIsImmediate,
    INS_OperandIsImplicit, 
    INS_OperandIsMemory, 
    INS_OperandIsReg, 
    INS_OperandRead, 
    INS_OperandReadAndWritten,
    INS_OperandReadOnly, 
    INS_OperandWritten, 
    INS_OperandWrittenOnly,
    INS_OperandReadAndWriten                    /* Not called */
};

static const char * testNames[] = {
    "&",
    "B",
    "F",
    "Imm",
    "Implicit",
    "Mem",
    "Reg",
    "R",
    "R/W",
    "Ro",
    "W",
    "Wo"
};

#define NumTestFunctions ((sizeof(testFunctions)/sizeof(testFunctions[0]))-1) // -1 to avoid INS_OperandReadAndWriten

/* Define the properties we want to test for a specific operand of a given instruction.
 * We define here the opcode, operand, which tests to run, and the expected result.
 * Additional tests could be added here. The only hard part is to find instructions which
 * have a restricted enough set of operands that you can know statically which operand you need to
 * look at.
 *
 * I was hoping that being able to choose the subset of operands to test would make this easier.
 * I haven't actually found a use for that, since every test so far uses "allTests". 
 *
 * On IA-32 Linux this covers all of the tests and requires that each gives both TRUE and FALSE answers.
 * On Intel64 (where you're unlikely to see INT), we're missing a test for IMMEDIATE. Since that's covered
 * on IA-32 I think it's OK.
 */
static struct instructionTest
{
    UINT32 opcode;                              /* Opcode */
    INT32 operand;                              /* Operand to test; if < 0 INS_OperandCount()-operand */
    UINT32 testProps;                           /* Mask of properties to test */
    UINT32 result;                              /* Expected result of the tests */

    instructionTest (UINT32 opc, UINT32 opd, UINT32 props, UINT32 res) :
        opcode(opc), operand(opd), testProps(props), result(res) {}

} tests [] = {
    instructionTest(XED_ICLASS_LEA,   0, allTests, isReg | isWrite | isWriteOnly ),
    instructionTest(XED_ICLASS_LEA,   1, allTests, isAddressGenerator | isRead | isReadOnly ),
    
    // Target memory operand of MOVS
    instructionTest(XED_ICLASS_MOVSD, 0, allTests, isFixedMemoOp | isMemory | isImplicit | isWrite | isWriteOnly ),
    // Target base register of MOVS
    instructionTest(XED_ICLASS_MOVSD, 1, allTests, isImplicit | isRead | isReadWrite | isWrite ),

#if (TARGET_IA32E)
    // Same test for MOVSQ
    instructionTest(XED_ICLASS_MOVSQ, 0, allTests, isFixedMemoOp | isMemory | isImplicit | isWrite | isWriteOnly ),
    instructionTest(XED_ICLASS_MOVSQ, 1, allTests, isImplicit | isRead | isReadWrite | isWrite ),
#endif

    // JZ, has  branch displacement
    instructionTest(XED_ICLASS_JZ, 0, allTests, isBranchDisplacement | isRead | isReadOnly),

    // INT has an immediate (at least, other than int3 which we don't expect to see)
    instructionTest(XED_ICLASS_INT, 0, allTests, isImmediate | isRead | isReadOnly),
};

#define NumTestDescriptions (sizeof(tests)/sizeof(tests[0]))

static string formatMask(UINT32 m)
{
    string r;
    BOOL first = TRUE;
    for (UINT32 i=0; i<NumTestFunctions; i++)
    {
        if (m & (1<<i))
        {
            if (first)
                first = FALSE;
            else
                r += ",";
            r += testNames[i];
        }
    }

    return r;
}

static struct {
    UINT32 tested;
    UINT32 failed;
} testCounts[XED_ICLASS_LAST];

static void Instruction(INS ins, VOID *)
{
    UINT32 op = INS_Opcode(ins);

    for (UINT32 j=0; j<NumTestDescriptions; j++)
    {
        instructionTest * t = &tests[j];

        if (t->opcode != op)
            continue;

        /* Aha, a test which applies to this opcode */
        UINT32 result = 0;
        INT32 operand = t->operand;
        
        if (operand < 0)
            operand = INS_OperandCount(ins) - operand;

        // Run all the property enquiries and accumulate the results.
        for (UINT32 i=0;i<NumTestFunctions;i++)
        {
            if (t->testProps & (1<<i))
            {
                if (testFunctions[i](ins, operand))
                    result |= (1<<i);
            }
        }
        testCounts[op].tested++;

        // Check the result against the expected value.
        if  (result != t->result)
        {
            testCounts[op].failed++;

            out << std::setw(50) << std::left << INS_Disassemble(ins) << std::right <<
                "Operand " << operand <<
                " Expected '" << formatMask(t->result) <<
                "' Got '" << formatMask(result) << "'" << endl;
        }
    }
}

static void AtEnd(INT32 code, VOID *arg)
{
    out << "Target exited with code : " << code << endl << endl;

    UINT32 failures = 0;

    for (UINT32 i=0; i<XED_ICLASS_LAST; i++)
    {
        if (testCounts[i].tested)
        {
            string mnemonic = OPCODE_StringShort(i);

            out << std::setw(12) << std::left << mnemonic << std::right <<
                " " << std::setw(9) << testCounts[i].tested << std::setw(9) << testCounts[i].failed;
            if (testCounts[i].failed)
                out << " <==ERROR==";
            out << endl;

            failures += testCounts[i].failed;
        }
    }
    out.close();
    exit(failures ? 1 : 0);
}

int main(int argc, char * argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    out.open(KnobOutput.Value().c_str());

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(AtEnd, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
