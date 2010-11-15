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
 * This is an example tool that adds extended debugger commands.  See the
 * Pin User Manual section "Debugging the Application while Running Under Pin"
 * for a tutorial about this tool.
 *
 * This tool adds extended commands to the debugger that allow you to set
 * breakpoints which trigger when the application's stack usage crosses a
 * user-specified threshold.  It also keeps track of stack usage statistics,
 * which can be displayed from within the debugger.  In GDB, type "monitor help"
 * to show a list of the commands this tool adds.
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cctype>
#include "pin.H"


// Command line switches for this tool.
//
KNOB<ADDRINT> KnobStackBreak(KNOB_MODE_WRITEONCE, "pintool",
    "stackbreak", "0",
    "Stop at breakpoint when thread uses this much stack (use with -appdebug_enable");
KNOB<std::string> KnobOut(KNOB_MODE_WRITEONCE, "pintool",
    "o", "",
    "When using -stackbreak, debugger connection information is printed to this file (default stderr)");
KNOB<UINT32> KnobTimeout(KNOB_MODE_WRITEONCE, "pintool",
    "timeout", "0",
    "When using -stackbreak, wait for this many seconds for debugger to connect (zero means wait forever)");


// Virtual register we use to point to each thread's TINFO structure.
//
static REG RegTinfo;


// Information about each thread.
//
struct TINFO
{
    ADDRINT _stackBase;     // Base (highest address) of stack.
    size_t _max;            // Maximum stack usage so far.
    size_t _maxReported;    // Maximum stack usage reported at breakpoint.
    std::ostringstream _os; // Used to format messages.
};

static std::ostream *Output = &std::cerr;
static bool EnableInstrumentation = false;
static bool BreakOnNewMax = false;
static ADDRINT BreakOnSize = 0;


static VOID OnThreadStart(THREADID, CONTEXT *, INT32, VOID *);
static VOID OnThreadEnd(THREADID, const CONTEXT *, INT32, VOID *);
static VOID Instruction(INS, VOID *);
static ADDRINT OnStackChangeIf(ADDRINT, ADDRINT);
static VOID DoBreakpoint(CONTEXT *, THREADID);
static void ConnectDebugger();
static BOOL DebugInterpreter(THREADID, CONTEXT *, const string &, string *, VOID *);
static std::string TrimWhitespace(const std::string &);


/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool demonstrates the use of extended debugger commands" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    if (PIN_GetDebugStatus() == DEBUG_STATUS_DISABLED)
    {
        std::cerr << "Application level debugging must be enabled to use this tool.\n";
        std::cerr << "Start Pin with either -appdebug or -appdebug_enable.\n";
        std::cerr << std::flush;
        return 1;
    }

    if (!KnobOut.Value().empty())
        Output = new std::ofstream(KnobOut.Value().c_str());

    if (KnobStackBreak.Value())
    {
        EnableInstrumentation = true;
        BreakOnSize = KnobStackBreak.Value();
    }

    // Allocate a virtual register that each thread uses to point to its
    // TINFO data.  Threads can use this virtual register to quickly access
    // their own thread-private data.
    //
    RegTinfo = PIN_ClaimToolRegister();
    if (!REG_valid(RegTinfo))
    {
        std::cerr << "Cannot allocate a scratch register.\n";
        std::cerr << std::flush;
        return 1;
    }
    PIN_AddDebugInterpreter(DebugInterpreter, 0);
    PIN_AddThreadStartFunction(OnThreadStart, 0);
    PIN_AddThreadFiniFunction(OnThreadEnd, 0);
    INS_AddInstrumentFunction(Instruction, 0);

    PIN_StartProgram();
    return 0;
}


/*
 * This call-back implements the extended debugger commands.
 *
 *  ctxt[in,out]    Register state for the debugger's "focus" thread.
 *  cmd[in]         Text of the extended command.
 *  result[out]     Text that the debugger prints when the command finishes.
 *
 * Returns: TRUE if we recognize this extended command.
 */
static BOOL DebugInterpreter(THREADID, CONTEXT *ctxt, const string &cmd, string *result, VOID *)
{
    TINFO *tinfo = reinterpret_cast<TINFO *>(PIN_GetContextReg(ctxt, RegTinfo));
    std::string line = TrimWhitespace(cmd);
    *result = "";

    if (line == "help")
    {
        result->append("stacktrace on        -- Enable tracing of stack usage.\n");
        result->append("stacktrace off       -- Disable tracing of stack usage.\n");
        result->append("stats                -- Show stack usage for current thread.\n");
        result->append("stackbreak newmax    -- Break when any thread stack reaches new maximum usage.\n");
        result->append("stackbreak <number>  -- Break when any thread stack usage exceeds <number> bytes.\n");
        result->append("stackbreak off       -- Disable stack breakpoints.\n");
        return TRUE;
    }
    else if (line == "stats")
    {
        size_t size = tinfo->_stackBase - PIN_GetContextReg(ctxt, REG_STACK_PTR);
        tinfo->_os.str("");
        tinfo->_os << "Current stack usage: " << std::dec << size << " bytes.\n";
        tinfo->_os << "Maximum stack usage: " << tinfo->_max << " bytes.\n";
        *result = tinfo->_os.str();
        return TRUE;
    }
    else if (line == "stacktrace on")
    {
        if (!EnableInstrumentation)
        {
            PIN_RemoveInstrumentation();
            EnableInstrumentation = true;
            *result = "Stack tracing enabled.\n";
        }
        return TRUE;
    }
    else if (line == "stacktrace off")
    {
        if (EnableInstrumentation)
        {
            PIN_RemoveInstrumentation();
            EnableInstrumentation = false;
            *result = "Stack tracing disabled.\n";
        }
        return TRUE;
    }
    else if (line == "stackbreak newmax")
    {
        BreakOnNewMax = true;
        BreakOnSize = 0;
        *result = "Will break when thread reaches new stack usage max.\n";
        return TRUE;
    }
    else if (line == "stackbreak off")
    {
        BreakOnNewMax = false;
        BreakOnSize = 0;
        return TRUE;
    }
    else if (line.find("stackbreak ") == 0)
    {
        std::istringstream is(&line.c_str()[sizeof("stackbreak ")-1]);
        size_t size;
        is >> size;
        if (!is)
        {
            *result = "Please specify a numeric size (in bytes)\n";
            return TRUE;
        }
        BreakOnNewMax = false;
        BreakOnSize = size;
        tinfo->_os.str("");
        tinfo->_os << "Will break when thread uses more than " << size << " bytes of stack.\n";
        *result = tinfo->_os.str();
        return TRUE;
    }

    return FALSE;   /* Unknown command */
}


static VOID OnThreadStart(THREADID, CONTEXT *ctxt, INT32, VOID *)
{
    TINFO *tinfo = new TINFO;
    tinfo->_stackBase = PIN_GetContextReg(ctxt, REG_STACK_PTR);
    tinfo->_max = 0;
    tinfo->_maxReported = 0;
    PIN_SetContextReg(ctxt, RegTinfo, reinterpret_cast<ADDRINT>(tinfo));
}

static VOID OnThreadEnd(THREADID, const CONTEXT *ctxt, INT32, VOID *)
{
    TINFO *tinfo = reinterpret_cast<TINFO *>(PIN_GetContextReg(ctxt, RegTinfo));
    delete tinfo;
}


static VOID Instruction(INS ins, VOID *)
{
    if (!EnableInstrumentation)
        return;

    if (INS_RegWContain(ins, REG_STACK_PTR))
    {
        IPOINT where = IPOINT_AFTER;
        if (!INS_HasFallThrough(ins))
            where = IPOINT_TAKEN_BRANCH;

        INS_InsertIfCall(ins, where, (AFUNPTR)OnStackChangeIf, IARG_REG_VALUE, REG_STACK_PTR,
            IARG_REG_VALUE, RegTinfo, IARG_END);
        INS_InsertThenCall(ins, where, (AFUNPTR)DoBreakpoint, IARG_CONTEXT, IARG_THREAD_ID, IARG_END);
    }
}


/*
 * Analysis routine called after each instruction that modifies the stack pointer.
 *
 *  sp[in]          Value of the stack pointer (after it is updated).
 *  addrInfo[in]    Address of the thread's TINFO structure.
 *
 * Returns: TRUE if the debugger should stop at a breakpoint.
 */
static ADDRINT OnStackChangeIf(ADDRINT sp, ADDRINT addrInfo)
{
    TINFO *tinfo = reinterpret_cast<TINFO *>(addrInfo);

    // The stack pointer may go above the base slightly.  (For example, the application's dynamic
    // loader does this briefly during start-up.)
    //
    if (sp > tinfo->_stackBase)
        return 0;

    // Keep track of the maximum stack usage.
    //
    size_t size = tinfo->_stackBase - sp;
    if (size > tinfo->_max)
        tinfo->_max = size;

    // See if we need to trigger a breakpoint.
    //
    if (BreakOnNewMax && size > tinfo->_maxReported)
        return 1;
    if (BreakOnSize && size >= BreakOnSize)
        return 1;
    return 0;
}


/*
 * Analysis routine called if we should stop at a breakpoint.
 *
 *  ctxt[in,out]    Appication register state immediately after the instruction that change $SP.
 *  tid[in]         Thread ID for thread that triggers the breakpoint.
 */
static VOID DoBreakpoint(CONTEXT *ctxt, THREADID tid)
{
    TINFO *tinfo = reinterpret_cast<TINFO *>(PIN_GetContextReg(ctxt, RegTinfo));

    // Keep track of the maximum reported stack usage for "stackbreak newmax".
    //
    size_t size = tinfo->_stackBase - PIN_GetContextReg(ctxt, REG_STACK_PTR);
    if (size > tinfo->_maxReported)
        tinfo->_maxReported = size;

    ConnectDebugger();  // Ask the user to connect a debugger, if it is not already connected.

    // Construct a string that the debugger will print when it stops.  If a debugger is
    // not connected, no breakpoint is triggered and execution resumes immediately.
    //
    tinfo->_os.str("");
    tinfo->_os << "Thread " << std::dec << tid << " uses " << size << " bytes of stack.";
    PIN_ApplicationBreakpoint(ctxt, tid, FALSE, tinfo->_os.str());
}


/*
 * If a debugger is not already connected, ask the user to connect one now.  Upon
 * return, a debugger may or may not be connected.
 */
static void ConnectDebugger()
{
    if (PIN_GetDebugStatus() == DEBUG_STATUS_CONNECTED)
        return;

    DEBUG_CONNECTION_INFO info;
    if (!PIN_GetDebugConnectionInfo(&info) || info._type != DEBUG_CONNECTION_TYPE_TCP_SERVER)
        return;

    *Output << "Triggered stack-limit breakpoint.\n";
    *Output << "Start GDB and enter this command:\n";
    *Output << "  target remote :" << std::dec << info._tcpServer._tcpPort << "\n";
    *Output << std::flush;

    if (PIN_WaitForDebuggerToConnect(1000*KnobTimeout.Value()))
        return;

    *Output << "No debugger attached after " << KnobTimeout.Value() << " seconds.\n";
    *Output << "Resuming application without stopping.\n";
    *Output << std::flush;
}


/*
 * Trim whitespace from a line of text.  Leading and trailing whitespace is removed.
 * Any internal whitespace is replaced with a single space (' ') character.
 *
 *  inLine[in]  Input text line.
 *
 * Returns: A string with the whitespace trimmed.
 */
static std::string TrimWhitespace(const std::string &inLine)
{
    std::string outLine = inLine;

    bool skipNextSpace = true;
    for (std::string::iterator it = outLine.begin();  it != outLine.end();  ++it)
    {
        if (std::isspace(*it))
        {
            if (skipNextSpace)
            {
                it = outLine.erase(it);
                if (it == outLine.end())
                    break;
            }
            else
            {
                *it = ' ';
                skipNextSpace = true;
            }
        }
        else
        {
            skipNextSpace = false;
        }
    }
    if (!outLine.empty())
    {
        std::string::reverse_iterator it = outLine.rbegin();
        if (std::isspace(*it))
            outLine.erase(outLine.size()-1);
    }
    return outLine;
}
