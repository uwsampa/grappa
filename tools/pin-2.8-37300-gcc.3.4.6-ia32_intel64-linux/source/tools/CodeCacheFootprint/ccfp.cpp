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
  @ORIGINAL_AUTHOR: Robert Cohn
*/

/* ===================================================================== */
/*! @file
 *  This file contains a static and dynamic opcode  mix profiler
 */



#include "pin.H"
#include "portability.H"
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,         "pintool",
    "o", "ccfp.out", "specify profile file name");
KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE,                "pintool",
    "i", "0", "append pid to output");
KNOB<BOOL>   KnobDynamic(KNOB_MODE_WRITEONCE,                "pintool",
    "dynamic", "1", "include dynamic statistics");

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool computes a code cache footprint\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}


/* ===================================================================== */
/* ===================================================================== */
typedef UINT64 COUNTER;


/* zero initialized */


class BBLSTATS
{
  public:
    COUNTER _counter;
    INT32 _numIns;

  public:
    BBLSTATS(INT32 numIns) : _counter(0), _numIns(numIns) {};

};


LOCALVAR vector<const BBLSTATS*> statsList;


/* ===================================================================== */

VOID docount(COUNTER * counter)
{
    (*counter) += 1;
}

// Tracks the number of times an original instruction has been put in the code cache
map<ADDRINT,INT32> OrigCount;

/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            if (OrigCount.find(INS_Address(ins)) == OrigCount.end())
                OrigCount[INS_Address(ins)] = 1;
            else
                OrigCount[INS_Address(ins)]++;
        }

        if (KnobDynamic)
        {
            // Insert instrumentation to count the number of times the bbl is executed
            BBLSTATS * bblstats = new BBLSTATS(BBL_NumIns(bbl));
            BBL_InsertCall(bbl, IPOINT_ANYWHERE, AFUNPTR(docount), IARG_PTR, &(bblstats->_counter), IARG_END);
            
            // Remember the counter and stats so we can compute a summary at the end
            statsList.push_back(bblstats);
        }
    }
}

UINT64 cacheInstructions = 0;

VOID TraceInserted(TRACE trace, VOID *)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            cacheInstructions++;
        }
    }
}

class CmpCount
{
  public:
    bool operator()(pair<ADDRINT,INT32> p1, pair<ADDRINT,INT32> p2)
    {
        return (p1.second > p2.second);
    }
};


/* ===================================================================== */
VOID DumpStats(ofstream& out)
{
    float cumBloat = 1.0;
    
    // Compute how many original instructions were entered into the code cache
    // and how many unique instructions are there
    INT32 uniqueInstructions = 0;
    INT32 totalInstructions = 0;
    for (map<ADDRINT,INT32>::iterator ci = OrigCount.begin(); ci != OrigCount.end(); ci++)
    {
        uniqueInstructions++;
        totalInstructions += ci->second;
    }
    
    float duplicateBloat = float(totalInstructions)/uniqueInstructions;
    cumBloat *= duplicateBloat;
    out << " (Original Fetched/Original unique) " << duplicateBloat
        << "(" << totalInstructions << "/" << uniqueInstructions << ")" << endl;

    INT32 executedInstructions = 0;
    INT32 tInstructions = 0;
    // Compute how many instructions that were entered into the code cache were never executed
    for (vector<const BBLSTATS*>::iterator si = statsList.begin(); si != statsList.end(); si++)
    {
        const BBLSTATS * stats = *si;
        
        tInstructions += stats->_numIns;
        
        if (stats->_counter > 0)
            executedInstructions += stats->_numIns;
    }
        
    float genBloat = float(cacheInstructions)/totalInstructions;
    cumBloat *= genBloat;
    out << " (Generated/Original Fetched) " << genBloat
        << "(" << cacheInstructions << "/" << totalInstructions << ")" << endl;

    if (KnobDynamic)
    {
        out << "WARNING: generated code includes instrumentation inserted by ccfp" << endl;
    }
    
    out << "Estimated cumulative static bloat: " << cumBloat << endl;

    float nonexecutedBloat = float(tInstructions)/executedInstructions;
    cumBloat *= nonexecutedBloat;
    out << " (Original fetched/Original Executed) " << nonexecutedBloat
        << "(" << tInstructions << "/" << executedInstructions << ")" << endl;

    out << "Estimated cumulative dynamic bloat: " << cumBloat << endl;

    float cumUniqueInstructions = 0;
    float cumTotalInstructions = 0;
    
    vector< pair<ADDRINT,INT32> > OrigCountSort;
    
    // cygwin does not like this
    //vector< pair<ADDRINT,INT32> > OrigCountSort(OrigCount.begin(), OrigCount.end());
    for (map<ADDRINT,INT32>::iterator ci = OrigCount.begin(); ci != OrigCount.end(); ci++)
        OrigCountSort.push_back(*ci);
    
    sort(OrigCountSort.begin(), OrigCountSort.end(), CmpCount());
    out << endl << "Number of times an original instruction appears in the code cache" << endl;
    out << "Address      Count        Unique     Total" << endl;
    out << left << setprecision(3);
    for (vector< pair<ADDRINT,INT32> >::iterator vi = OrigCountSort.begin();
         vi != OrigCountSort.end();
         vi++)
    {
        cumUniqueInstructions++;
        cumTotalInstructions += vi->second;
        out << hex << setw(16) << vi->first << " "
            << dec << setw(8) << vi->second << " "
            << setw(10) << cumUniqueInstructions/uniqueInstructions << " " 
            << setw(10) << cumTotalInstructions/totalInstructions << endl;
    }
}



/* ===================================================================== */

VOID Fini(int, VOID * v)
{
    string filename =  KnobOutputFile.Value();

    if( KnobPid )
    {
        filename += "." + decstr( getpid_portable() );
    }

    std::ofstream out(filename.c_str());

    DumpStats(out);
    
    out << "# $eof" <<  endl;

    out.close();
}

    
/* ===================================================================== */

int main(int argc, CHAR *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    TRACE_AddInstrumentFunction(Trace, 0);

    CODECACHE_AddTraceInsertedFunction(TraceInserted, 0);
    
    PIN_AddFiniFunction(Fini, 0);

    // Never returns

    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
