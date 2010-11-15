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
//  ORIGINAL_AUTHOR: Kim Hazelwood
//
//  This tool demonstrates the integration of Pin and PAPI
//   by gathering basic instruction and cycle counts
//  Sample usage:
//    pin -t papiDemo -- /bin/ls

#include "pin.H"
#include "utils.H"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <papi.h>

using namespace std;

/* ================================================================== */
/* Global Variables                                                   */
/* ================================================================== */

ofstream TraceFile;

/* ================================================================== */
/* Command-Line Switches                                              */
/* ================================================================== */

KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE, "pintool",
    "p", "1", "append pid to output");
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "papiDemo.out", "specify trace file name");

int EventSet;
long long values[10];

/* ================================================================== */
/*
 Use PAPI to start the counters
*/
VOID StartPapiCounters()
{
   EventSet = PAPI_NULL;

   if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
   {
       cout << "Error in PAPI_library_init!\n";
       exit(1);
   }
   if (PAPI_create_eventset(&EventSet) != PAPI_OK)
   {
       cout << "Error in PAPI_create_eventset!\n";
       exit(1);
   }
   if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK)
   {
       cout << "Error in PAPI_add_event PAPI_TOT_INS!\n";
       exit(2);
   }
   if (PAPI_add_event(EventSet, PAPI_TOT_CYC) != PAPI_OK)
   {
       cout << "Error in PAPI_add_event PAPI_TOT_CYC!\n";
       exit(3);
   }
   if (PAPI_start(EventSet) != PAPI_OK)
   {
       cout << "Error in PAPI_start!\n";
       exit(4);
   }
}

/* ================================================================== */
/*
 Use PAPI to stop the counters.  Print details at the end of the run
*/
VOID StopAndPrintPapiCounters(INT32 code, VOID *v)
{
    string logFileName = KnobOutputFile.Value();
    if( KnobPid )
    {
        logFileName += "." + decstr( PIN_GetPid() );
    }
    TraceFile.open(logFileName.c_str());
    
    TraceFile << dec << "\n--------------------------------------------\n";
    if (PAPI_stop(EventSet, values) != PAPI_OK)
    {
        TraceFile << "Error in PAPI_stop!\n";
    }
    TraceFile << "Total Instructions " << values[0] << " Total Cycles " << values[1] << endl;

    TraceFile << dec << "--------------------------------------------\n";
    TraceFile << "Code cache size: ";
    int cacheSizeLimit = CODECACHE_CacheSizeLimit();
    if (cacheSizeLimit)
        TraceFile << BytesToString(cacheSizeLimit) << endl;
    else
        TraceFile << "UNLIMITED" << endl;
    TraceFile << BytesToString(CODECACHE_CodeMemUsed()) << " used" << endl;
    TraceFile << "#eof" << endl;
    TraceFile.close();
}

/* ================================================================== */
/*
 Initialize and begin program execution under the control of Pin
*/
int main(INT32 argc, CHAR **argv)
{
    if (PIN_Init(argc, argv)) return Usage();

    // Register a routine that gets called when the cache is first initialized
    CODECACHE_AddCacheInitFunction(StartPapiCounters, 0);
    
    // Register routines that get called when the program ends
    PIN_AddFiniFunction(StopAndPrintPapiCounters, 0);
    
    PIN_StartProgram();  // Never returns
    
    return 0;
}
