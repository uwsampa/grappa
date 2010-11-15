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
  @ORIGINAL_AUTHOR: Elena Demikhovsky
*/

/* ===================================================================== */
/*! @file
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sched.h>
#include <assert.h>

using namespace std;


/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "probe_tool.out", "specify file name");

ofstream TraceFile;
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool tests MT attach in probe mode.\n"
        "\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

PIN_LOCK lock;

UINT32 threadCounter=0;
BOOL   isAppStartReceived = FALSE;

VOID AppStart(VOID *v)
{
    GetLock(&lock, PIN_GetTid());
    TraceFile << "Application Start Callback is called from thread " << dec << PIN_GetTid() << endl;
    isAppStartReceived = TRUE;
    ReleaseLock(&lock);
}

VOID AttachedThreadStart(VOID *v)
{
    GetLock(&lock, PIN_GetTid());
    TraceFile << "Thread counter is updated to " << dec <<  (threadCounter+1) << endl;
    ++threadCounter;
    ReleaseLock(&lock);
}

int PinReady(unsigned int numOfThreads)
{
    GetLock(&lock, PIN_GetTid());
	// Check that we don't have any extra thread
	assert(threadCounter <= numOfThreads);
    if ((threadCounter == numOfThreads) && isAppStartReceived)
    {
        TraceFile.close();
        ReleaseLock(&lock);
        return 1;
    }
    ReleaseLock(&lock);
    return 0;
}

VOID ImageLoad(IMG img, void *v)
{
	RTN rtn = RTN_FindByName(img, "ThreadsReady");
	if (RTN_Valid(rtn))
	{
		RTN_ReplaceProbed(rtn, AFUNPTR(PinReady));
	}
}	


/* ===================================================================== */

int main(int argc, CHAR *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    TraceFile.open(KnobOutputFile.Value().c_str());
    TraceFile << hex;
    TraceFile.setf(ios::showbase);

	InitLock(&lock);
	
    IMG_AddInstrumentFunction(ImageLoad, 0);
    PIN_AddApplicationStartFunction(AppStart, 0);
    PIN_AddThreadAttachProbedFunction(AttachedThreadStart, 0);
    PIN_StartProgramProbed();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
