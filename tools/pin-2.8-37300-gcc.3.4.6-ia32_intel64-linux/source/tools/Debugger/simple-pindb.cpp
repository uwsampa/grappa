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
 * This application is a general test of features in  the 'pindb' debugger.
 * Since the debugger is not symbolic, the input commands must reference raw
 * addresses in this application not symbol names.  Therefore, it would be
 * difficult to keep the addresses in the input commands in sync with
 * addresses in this application.  To solve this problem, the application
 * itself can print out the debugger commands.  Therefore, to run the test,
 * you run this application twice.  The first run generates the debugger
 * command script and the second run executes under the debugger.
 */

#include <iostream>
#include <fstream>
#include <string>

#if defined(_MSC_VER)
    typedef unsigned __int64 ADDR;
#elif defined(__GNUC__)
    #include <stdint.h>
    typedef uint64_t ADDR;
#endif


static void GenerateScripts(const char *, const char *);
extern "C" void Breakpoint();
extern "C" void Breakpoint2();
extern "C" void DoRegMemTest();
extern "C" long MemTestData[];


int main(int argc, char **argv)
{
    if (argc != 1 && argc != 3)
    {
        std::cerr << "Must specify two arguments or none\n";
        return 1;
    }

    // When arguments are specified, just generate the
    // debugger scripts.
    //
    if (argc == 3)
    {
        GenerateScripts(argv[1], argv[2]);
        return 0;
    }

    // When run with no arguments, execute the test code.
    //
    Breakpoint();
    Breakpoint();
    Breakpoint();
    DoRegMemTest();
    return 0;
}


// Generate the input script to use when debugging this program with "pindb".
// This routine generates two files: 'inFile' holds the debugger input commands.
// and 'compareFile' holds the expected debugger output.
//
static void GenerateScripts(const char *inFile, const char *compareFile)
{
    std::ofstream in(inFile);
    std::ofstream compare(compareFile);

    ADDR bpAddr1 = reinterpret_cast<ADDR>(&Breakpoint);
    ADDR bpAddr2 = reinterpret_cast<ADDR>(&Breakpoint2);
    ADDR memTestAddr = reinterpret_cast<ADDR>(&DoRegMemTest);
    ADDR memDataAddr = reinterpret_cast<ADDR>(&MemTestData[0]);

    in << std::hex << std::noshowbase;
    compare << std::hex << std::noshowbase;

    compare << "Thread .* stopped at\n";

    // Test breakpoints
    //
    in << "b 0x" << bpAddr1 << "\n";
    in << "b 0x" << bpAddr2 << "\n";
    in << "c\n";
    compare << "Thread .* stopped at 0x" << bpAddr1 << " \\(at breakpoint\\)\n";
    in << "c\n";
    compare << "Thread .* stopped at 0x" << bpAddr2 << " \\(at breakpoint\\)\n";
    in << "c\n";
    compare << "Thread .* stopped at 0x" << bpAddr1 << " \\(at breakpoint\\)\n";
    in << "s\n";
    compare << "Thread .* stopped at 0x" << bpAddr2 << "\n";

    // Test disassembly
    //
    in << "dis 2 0x" << bpAddr1 << "\n";
    compare << "0x0*" << bpAddr1 << ".*nop\n";
    compare << "0x0*" << bpAddr2 << ".*ret\n";

    // Test clearing breakpoints.  Set up for register & memory tests.
    //
    in << "del 0x" << bpAddr1 << "\n";
    in << "del 0x" << bpAddr2 << "\n";
    in << "b 0x" << memTestAddr << "\n";

    // Test examine memory.
    //
    in << "x/2w 0x" << memDataAddr << "\n";
    compare << "0x0*" << memDataAddr << "  12345678 deadbeef\n";

    in << "c\n";
    compare << "Thread .* stopped at 0x" << memTestAddr << " \\(at breakpoint\\)\n";
    in << "s\n";
    in << "s\n";
#if defined(TARGET_IA32E)
    std::string reg = "rcx";
#elif defined(TARGET_IA32)
    std::string reg = "ecx";
#else
# error "Need TARGET_IA32 or TARGET_IA32E defined"
#endif
    in << "p $" << reg << "\n";
    compare << "0x0*12345678\n";
    in << "set $" << reg << "=0x123fff\n";

    in << "s\n";

    // Examine the modified memory (verifies that we really did change %cx).
    //
    in << "x/2w 0x" << memDataAddr << "\n";
    compare << "0x0*" << memDataAddr << "  00123fff deadbeef\n";

    // Test that we can modify memory.
    //
    in << "m/1w 0x" << memDataAddr << " 0xccc456\n";
    in << "x/2w 0x" << memDataAddr << "\n";
    compare << "0x0*" << memDataAddr << "  00ccc456 deadbeef\n";

    // Test that we can send pintool-commands and receive a response.
    //
    in << "monitor stacktrace on\n";
    compare << "Stack tracing enabled.\n";
    in << "monitor stacktrace off\n";
    compare << "Stack tracing disabled.\n";

    in << "c\n";
    compare << "Program exited with 0\n";
    in << "q\n";
}
