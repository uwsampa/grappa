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
#include <fstream>
#include <map>
#include <set>
#include "pin.H"

#include <unistd.h>
#include <sys/times.h>

LOCALVAR ADDRINT GlobalLow;
LOCALVAR ADDRINT GlobalHigh;
LOCALVAR KNOB<INT32>   KnobSample(KNOB_MODE_WRITEONCE,                "pintool",
                              "sample", "0", "number of executions to monitor");

LOCALVAR KNOB<string>  KnobOutputFile(KNOB_MODE_WRITEONCE,                "pintool",
                              "o", "deprof.out", "output file");

LOCALTYPE class REFINFO
{
  public:
    REFINFO() {_global = 0;_nonglobal = 0;};
    
    UINT64 _global;
    UINT64 _nonglobal;
};
    
LOCALTYPE typedef map<ADDRINT,REFINFO*> GLOBALREFS;

LOCALVAR GLOBALREFS GlobalRefs;

LOCALFUN BOOL GlobalData(ADDRINT address)
{
    return (address >= GlobalLow
            && address < GlobalHigh);
}

VOID Img(IMG img, VOID *)
{
    if (IMG_Type(img) == IMG_TYPE_SHAREDLIB)
        return;

    GlobalLow = IMG_LowAddress(img);
    GlobalHigh = IMG_HighAddress(img);
}

LOCALTYPE typedef class MEMREF
{
  public:
    ADDRINT _address;
    REFINFO * _ri;
};


LOCALTYPE typedef class TRACEINFO
{
  public:
    TRACEINFO(ADDRINT traceAddress, INT32 count) {
        _traceAddress = traceAddress;
        _count = count;
        _expired = false;
        _codeCacheSize = 0;
    };
    
    INT64 _count;
    ADDRINT _traceAddress;
    INT32 _codeCacheSize;
    BOOL _expired;
};

// Traces
typedef map<ADDRINT,TRACEINFO*> TRACES;
LOCALVAR TRACES Traces;

LOCALCONST INT32 MaxBufferRefs = 1000;

LOCALVAR MEMREF MemRefBuffer[MaxBufferRefs];
LOCALVAR MEMREF * BufferTop = MemRefBuffer;

LOCALVAR UINT32 ExpiredTracesCodeCacheSize = 0;
LOCALVAR UINT32 TracesCodeCacheSize = 0;

LOCALCONST MEMREF * BufferFull = MemRefBuffer + MaxBufferRefs;

LOCALFUN ADDRINT BufferOverflow(INT32 entries)
{
    return BufferTop + entries >= BufferFull;
}

LOCALFUN BOOL TraceExpire(TRACEINFO * ti)
{
    ti->_count--;
    return ti->_count == 0;
}

LOCALFUN ADDRINT BufferOverflowOrTraceExpire(INT32 entries, TRACEINFO * ti)
{
    return BufferOverflow(entries) + TraceExpire(ti);
}

LOCALFUN VOID WatchAddress(ADDRINT address, REFINFO * ri)
{
    UINT64 * counter = (GlobalData(address) ? &(ri->_global) : &(ri->_nonglobal));
    (*counter)++;
}

LOCALFUN VOID ProcessBuffer(INT32 refs, TRACEINFO * ti)
{
    if (KnobSample > 0
        && ti->_count < 0)
    {
        ti->_expired = true;
        
        ExpiredTracesCodeCacheSize += ti->_codeCacheSize;
        
        //cerr << "Invalidate " << hex << ti->_traceAddress << endl;
        CODECACHE_InvalidateTraceAtProgramAddress(ti->_traceAddress);
    }
    
    for (MEMREF *mr = MemRefBuffer; mr < BufferTop; mr++)
    {
        WatchAddress(mr->_address, mr->_ri);
    }

    BufferTop = MemRefBuffer;

    ASSERTX(!BufferOverflow(refs));
}

LOCALFUN VOID RecordAddress(ADDRINT address, REFINFO * ri)
{
    MEMREF * mr = BufferTop;
    
    //ASSERTX(mr < &(MemRefBuffer[MaxBufferRefs]));
    
    mr->_address = address;
    mr->_ri = ri;

    BufferTop++;
}

LOCALFUN BOOL InterestingMemory(INS ins)
{
    if (!INS_IsMemoryRead(ins) && !INS_IsMemoryWrite(ins))
        return false;

    if (INS_IsStackRead(ins)
        || INS_IsStackWrite(ins)
        || INS_IsIpRelRead(ins)
        || INS_IsIpRelWrite(ins))
        return false;
    
    // Absolute addressing
    for (UINT32 i = 0; i < INS_OperandCount(ins); i++)
    {
        if (!INS_OperandIsMemory(ins, i))
            continue;

        if (INS_OperandMemoryBaseReg(ins, i) == REG_INVALID()
            && INS_OperandMemoryIndexReg(ins, i) == REG_INVALID())
            return false;

        // A huge displacement is probably a base address
        if (GlobalData(INS_OperandMemoryDisplacement(ins, i)))
            return false;
    }

    return true;
}
            
VOID InstrumentMemory(INS ins)
{
    if (!InterestingMemory(ins))
        return;
    
    GLOBALREFS::iterator gi = GlobalRefs.find(INS_Address(ins));
    REFINFO * ri;
    
    if (gi == GlobalRefs.end())
    {
        ri = new REFINFO;
        
        GlobalRefs[INS_Address(ins)] = ri;
    }
    else
    {
        ri = gi->second;
    }

    if (INS_IsMemoryRead(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(RecordAddress), IARG_MEMORYREAD_EA, IARG_PTR, ri, IARG_END);
    }
    if (INS_IsMemoryWrite(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(RecordAddress), IARG_MEMORYWRITE_EA, IARG_PTR, ri, IARG_END);
    }
    if (INS_HasMemoryRead2(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(RecordAddress), IARG_MEMORYREAD2_EA, IARG_PTR, ri, IARG_END);
    }
}

LOCALFUN INT32 MaxMemRefs(TRACE trace)
{
    INT32 refs = 0;
    
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            if (InterestingMemory(ins))
            {
                if (INS_IsMemoryRead(ins))
                {
                    refs++;
                }
                if (INS_IsMemoryWrite(ins))
                {
                    refs++;
                }
                if (INS_HasMemoryRead2(ins))
                {
                    refs++;
                }
            }
        }
    }

    return refs;
}

    
VOID Trace(TRACE trace, VOID *)
{
    // Nothing to profile
    INT32 refs = MaxMemRefs(trace);
    if (refs == 0)
        return;
    
    // Has this trace expired?
    ADDRINT address = TRACE_Address(trace);
    TRACES::iterator tii = Traces.find(address);
    TRACEINFO * ti = 0;
    if (tii != Traces.end())
    {
        ti = tii->second;

        if (ti->_expired)
            return;
    }
    else
    {
        ti = new TRACEINFO(address, KnobSample);
        //cerr << "Inserting trace at " << hex << address << endl;
        Traces.insert(pair<ADDRINT,TRACEINFO*>(address,ti));
    }
    
    
    // Make sure there is enough room in the buffer
    if (KnobSample > 0)
    {
        TRACE_InsertIfCall(trace, IPOINT_BEFORE, AFUNPTR(BufferOverflowOrTraceExpire),
                           IARG_UINT32, refs,
                           IARG_PTR, ti,
                           IARG_END);
    }
    else
    {
        TRACE_InsertIfCall(trace, IPOINT_BEFORE, AFUNPTR(BufferOverflow), IARG_UINT32, refs, IARG_END);
    }
    
    TRACE_InsertThenCall(trace, IPOINT_BEFORE, AFUNPTR(ProcessBuffer),
                         IARG_UINT32, refs,
                         IARG_PTR, ti,
                         IARG_END);
    
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            InstrumentMemory(ins);
        }
    }
}

LOCALFUN VOID Fini(INT32 code, VOID *)
{
    ofstream ofile(KnobOutputFile.Value().c_str());
    
    struct tms time;
    times(&time);
    
    ofile << "CpuTime " << dec << (float)time.tms_utime/(float)sysconf(_SC_CLK_TCK) << endl;
    ofile << "ExpiredTracesCodeCacheSize " << dec << ExpiredTracesCodeCacheSize << endl;
    ofile << "TracesCodeCacheSize " << dec << TracesCodeCacheSize << endl;
    ofile << "BeginProfile" << endl;
    
    for (GLOBALREFS::const_iterator gi = GlobalRefs.begin(); gi != GlobalRefs.end(); gi++)
    {
        REFINFO * ri = gi->second;
        ofile << hex << gi->first << " " << dec << ri->_global << " " << ri->_nonglobal << endl;
    }
}


VOID TraceInserted(TRACE trace, VOID *)
{
    TRACES::iterator tii = Traces.find(TRACE_Address(trace));

    if (tii == Traces.end())
    {
        // Trace was not interesting
        //cerr << "No trace in TRACES " << hex << TRACE_Address(trace) << endl;
        return;
    }
    
    ASSERTX(tii != Traces.end());

    TRACEINFO *ti = tii->second;

    ti->_codeCacheSize += TRACE_CodeCacheSize(trace);
    TracesCodeCacheSize += TRACE_CodeCacheSize(trace);
}
                       
int main(int argc, char * argv[])
{
    PIN_Init(argc, argv);

    IMG_AddInstrumentFunction(Img, 0);
    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddFiniFunction(Fini, 0);
    CODECACHE_AddTraceInsertedFunction(TraceInserted, 0);
    
    PIN_StartProgram();
    
    return 0;
}
