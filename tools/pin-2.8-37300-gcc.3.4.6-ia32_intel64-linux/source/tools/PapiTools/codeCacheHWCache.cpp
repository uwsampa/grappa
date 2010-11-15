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
//  This tool collects information about the code cache while
//      using PAPI to collect information about the HW 
//      cache misses.
//  Sample usage:
//    pin -t codeCacheHWCache -- /bin/ls

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

struct GLOBALSTRUCT
{
    int flushes;
    int jumpsIntoCache;
    int cacheblocks;
    int insertions;
    int numLinks;
}Global;
ofstream TraceFile;

/* ================================================================== */
/* Command-Line Switches                                              */
/* ================================================================== */

KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE, "pintool",
    "p", "1", "append pid to output");
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "codeCacheHWCache.out", "specify trace file name");

/* ================================================================== */
/*
 Clear the cache flush count
*/
VOID InitStats()
{
    Global.flushes = 0;
    Global.jumpsIntoCache = 0;
    Global.cacheblocks = 0;
    Global.insertions = 0;
    Global.numLinks = 0;
}

/* ================================================================== */
/*
  When notified by Pin that the cache is full, perform a flush and
  tell the user about it.
*/
VOID FlushOnFull(UINT32 trace_size, UINT32 stub_size)
{
    Global.flushes++;
    CODECACHE_FlushCache();
}

/* ================================================================== */
/*
 Print details of jumps into code cache
*/
VOID WatchEnter(ADDRINT cache_pc)
{
    Global.jumpsIntoCache++;
}

/* ================================================================== */
/*
 Print details of links as they are patched
*/
VOID WatchLinks(ADDRINT branch_pc, ADDRINT target_pc)
{
    Global.numLinks++;
}

/* ================================================================== */
/*
 Print details of traces as they are inserted
*/
VOID WatchTraces(TRACE trace, VOID *v)
{
    Global.insertions++;
}

int EventSet;
long long values[2];

/* ================================================================== */
/*
 Use PAPI to start the counters
*/
VOID StartIcacheCounter()
{
   EventSet = PAPI_NULL;

   if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
   {
       cout << "Error in PAPI_library_init!\n";
       cout << "Verify that you are running a perfctr kernel and that PAPI is properly installed.\n";
       exit(1);
   }
   if (PAPI_create_eventset(&EventSet) != PAPI_OK)
   {
       cout << "Error in PAPI_create_eventset!\n";
       exit(1);
   }
   if (PAPI_add_event(EventSet, PAPI_L1_ICM) != PAPI_OK)
   {
       cout << "Error in PAPI_add_event PAPI_L1_ICM!\n";
       cout << "Verify that your processor supports the collection of instruction cache misses.\n";
       exit(2);
   }
   if (PAPI_add_event(EventSet, PAPI_L1_ICA) != PAPI_OK)
   {
       cout << "Error in PAPI_add_event PAPI_L1_ICA!\n";
       cout << "Verify that your processor supports the collection of instruction cache accesses.\n";
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
VOID PrintDetailsOnExit(INT32 code, VOID *v)
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
    TraceFile << "L1 Icache Misses " << values[0] << " Accesses " << values[1] << endl;

    TraceFile << dec << "--------------------------------------------\n";
    TraceFile << Global.insertions << " insertions into code cache" << endl;
    TraceFile << Global.jumpsIntoCache << " control transfers into cache\n";
    TraceFile << Global.numLinks << " links set in the code cache" << endl;
    TraceFile << "--------------------------------------------\n";
    TraceFile << "Code cache size: ";
    int cacheSizeLimit = CODECACHE_CacheSizeLimit();
    if (cacheSizeLimit)
        TraceFile << BytesToString(cacheSizeLimit) << endl;
    else
        TraceFile << "UNLIMITED" << endl;
    TraceFile << BytesToString(CODECACHE_CodeMemUsed()) << " used" << endl;
    TraceFile << Global.cacheblocks << " cache blocks created" << endl;
    TraceFile << Global.flushes << " flushes!" << endl;
    TraceFile << "#eof" << endl;
    TraceFile.close();
}

/* ================================================================== */
/*
  Print details of a cache allocation
*/
VOID CollectBlockInfo(UINT32 block_size )
{
    Global.cacheblocks++;
}

/* ================================================================== */
/*
 Initialize and begin program execution under the control of Pin
*/
int main(INT32 argc, CHAR **argv)
{
    if (PIN_Init(argc, argv)) return Usage();

    // Initialize some local data structures
    InitStats();
    
    // Register a routine that gets called when the cache is first initialized
    CODECACHE_AddCacheInitFunction(StartIcacheCounter, 0);
    
    // Register a routine that gets called when new cache blocks are formed
    CODECACHE_AddCacheBlockFunction(CollectBlockInfo, 0);

    // Register a routine that gets called when we jump into 
    //  the code cache
    CODECACHE_AddCodeCacheEnteredFunction(WatchEnter, 0);

    // Register a routine that gets called when a trace is 
    //  inserted into the codecache
    CODECACHE_AddTraceInsertedFunction(WatchTraces, 0);

    // Register a routine that gets called when patch a link in
    //  the code cache
    CODECACHE_AddTraceLinkedFunction(WatchLinks, 0);

    // Register a routine that gets called every time the cache is full
    CODECACHE_AddFullCacheFunction(FlushOnFull, 0);
    
    // Register routines that get called when the program ends
    PIN_AddFiniFunction(PrintDetailsOnExit, 0);
    
    PIN_StartProgram();  // Never returns
    
    return 0;
}
