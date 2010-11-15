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
 * Sample buffering tool
 * 
 * This tool collects an address trace of instructions that access memory
 * by filling a buffer.  When the buffer overflows,the callback writes all
 * of the collected records to a file.
 *
 */
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "pin.H"
#include "portability.H"
using namespace std;

/*
 * Name of the output file
 */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "buffer.out", "output file");

/*
 * The ID of the buffer
 */
BUFFER_ID bufId;

/*
 * Thread specific data
 */
TLS_KEY mlog_key;

/*
 * Number of OS pages for the buffer
 */
#define NUM_BUF_PAGES 1024

/*
 * Record of memory references.  Rather than having two separate
 * buffers for reads and writes, we just use one struct that includes a
 * flag for type.
 */
struct MEMREF
{
    ADDRINT     pc;
    ADDRINT     ea;
    UINT32      size;
    BOOL        read;
};

/*
 * MLOG - thread specific data that is not handled by the buffering API.
 */
class MLOG
{
  public:
    MLOG(THREADID tid);
    ~MLOG();
    
    VOID DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid );
    
  private:
    ofstream _ofile;
};

MLOG::MLOG(THREADID tid)
{
    string filename = KnobOutputFile.Value() + "." + decstr(getpid_portable()) + "." + decstr(tid);
    
    _ofile.open(filename.c_str());
    
    if ( ! _ofile )
    {
        cerr << "Error: could not open output file." << endl;
        exit(1);
    }
    
    _ofile << hex;
}

MLOG::~MLOG()
{
    _ofile.close();
}

VOID MLOG::DumpBufferToFile( struct MEMREF * reference, UINT64 numElements, THREADID tid )
{
    for(UINT64 i=0; i<numElements; i++, reference++)
    {
        if (reference->ea != 0)
            _ofile << reference->pc << "   " << reference->ea << endl;
    }
}

/**************************************************************************
 *
 *  Instrumentation routines
 *
 **************************************************************************/

int predicate( int count )
{
    return count;
}

/*
 * Insert code to write data to a thread-specific buffer for instructions
 * that access memory.
 */
VOID Trace(TRACE trace, VOID *v)
{
    UINT32 refSize;
    
    for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl=BBL_Next(bbl))
    {
        for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins=INS_Next(ins))
        {
            if(INS_IsMemoryRead(ins))
            {
                refSize = INS_MemoryReadSize(ins);
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)predicate, IARG_UINT32, 1, IARG_END);
                INS_InsertFillBufferThen(ins, IPOINT_BEFORE, bufId,
                                     IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                     IARG_MEMORYREAD_EA, offsetof(struct MEMREF, ea),
                                     IARG_UINT32, refSize, offsetof(struct MEMREF, size),
                                     IARG_BOOL, TRUE, offsetof(struct MEMREF, read),
                                     IARG_END);
            }
            
            if (INS_HasMemoryRead2(ins))
            {
                refSize = INS_MemoryReadSize(ins);
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)predicate, IARG_UINT32, 1, IARG_END);
                INS_InsertFillBufferThen(ins, IPOINT_BEFORE, bufId,
                                     IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                     IARG_MEMORYREAD2_EA, offsetof(struct MEMREF, ea),
                                     IARG_UINT32, refSize, offsetof(struct MEMREF, size),
                                     IARG_BOOL, TRUE, offsetof(struct MEMREF, read),
                                     IARG_END);
            }
            
            if(INS_IsMemoryWrite(ins))
            {
                refSize = INS_MemoryWriteSize(ins);
                
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)predicate, IARG_UINT32, 0, IARG_END);
                INS_InsertFillBufferThen(ins, IPOINT_BEFORE, bufId,
                                     IARG_INST_PTR, offsetof(struct MEMREF, pc),
                                     IARG_MEMORYWRITE_EA, offsetof(struct MEMREF, ea),
                                     IARG_UINT32, refSize, offsetof(struct MEMREF, size),
                                     IARG_BOOL, FALSE, offsetof(struct MEMREF, read),
                                     IARG_END);
            }
        }
    }
}

/**************************************************************************
 *
 *  Callback Routines
 *
 **************************************************************************/

/*!
 * Called when a buffer fills up, or the thread exits, so we can process it or pass it off
 * as we see fit.
 * @param[in] id		buffer handle
 * @param[in] tid		id of owning thread
 * @param[in] ctxt		application context
 * @param[in] buf		actual pointer to buffer
 * @param[in] numElements	number of records
 * @param[in] v			callback value
 * @return  A pointer to the buffer to resume filling.
 */
VOID * BufferFull(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf,
                  UINT64 numElements, VOID *v)
{
    struct MEMREF * reference=(struct MEMREF*)buf;
    
    MLOG * mlog = static_cast<MLOG*>( PIN_GetThreadData( mlog_key, tid ) );
    
    mlog->DumpBufferToFile( reference, numElements, tid );
    
    return buf;
}

/*
 * Note that opening a file in a callback is only supported on Linux systems.
 * See buffer-win.cpp for how to work around this issue on Windows.
 */
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // There is a new MLOG for every thread.  Opens the output file.
    MLOG * mlog = new MLOG(tid);
    
    // A thread will need to look up its MLOG, so save pointer in TLS
    PIN_SetThreadData(mlog_key, mlog, tid);
}

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    MLOG * mlog = static_cast<MLOG*>(PIN_GetThreadData(mlog_key, tid));
    delete mlog;
    
    PIN_SetThreadData(mlog_key, 0, tid);
}

/**************************************************************************
 *
 *  Main
 *
 **************************************************************************/
/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool demonstrates the basic use of the buffering API." << endl << endl;
    return -1;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments,
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Initialize the memory reference buffer;
    // set up the callback to process the buffer.
    //
    bufId = PIN_DefineTraceBuffer(sizeof(struct MEMREF), NUM_BUF_PAGES,
                                  BufferFull, 0);
    
    if(bufId == BUFFER_ID_INVALID)
    {
        cerr << "Error: could not allocate initial buffer" << endl;
        return 1;
    }
    
    // Initialize thread-specific data not handled by buffering api.
    mlog_key = PIN_CreateThreadDataKey(0);
    
    // add an instrumentation function
    TRACE_AddInstrumentFunction(Trace, 0);
    
    // add callbacks
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
