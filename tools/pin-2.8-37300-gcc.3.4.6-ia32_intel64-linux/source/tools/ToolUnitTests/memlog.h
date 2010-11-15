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
/* ================================================================== */
/* Memory Logging Functionalities:                                    */
/* RecordRead     remembers the memory locations that have been read  */
/* RecordWrite    records original mem state before any changes occur */
/* RestoreMem     restores the memory to the saved original state     */
/* DetectConflict determines whether the given memop conflicts w/ log */
/* Reset          clears all the logs                                 */
/* DumpMemState   dumps the current mem state & clears the writelog   */
/* InitWriteLog   creates a write log based on the dump file          */
/* ================================================================== */

#ifndef PINTOOL_MEMLOG_H
#define PINTOOL_MEMLOG_H

#include <map>
#include <set>
#include "utilities.h"

class MEMLOG
{
  public:
    MEMLOG(BOOL _verbose) : verbose(_verbose) {}
    VOID RecordRead(ADDRINT, UINT32, fstream&);
    VOID RecordWrite(ADDRINT, UINT32, fstream&);
    VOID RestoreMem(fstream&);
    BOOL DetectConflict(ADDRINT, UINT32, BOOL, fstream&);
    VOID Reset();
    VOID DumpMemState(fstream&, fstream&, ADDRINT);
    VOID InitWriteLog(fstream&, fstream&);
  private:
    set<ADDRINT> readlog;
    map<ADDRINT, UINT8> writelog;
    set<ADDRINT> stacklog;
    BOOL verbose;
};

VOID MEMLOG::RecordRead(ADDRINT addr, UINT32 len, fstream& tracefile) 
{
    for (ADDRINT addrbyte = addr; addrbyte < (addr + len); addrbyte++) 
    {
        readlog.insert(addrbyte);
#if 0
        if (verbose) 
        {
            tracefile << "Remembering Read:\tLocation = " << hex << addrbyte << "\n" << flush;
        }
#endif
    }
}

VOID MEMLOG::RecordWrite(ADDRINT addr, UINT32 len, fstream& tracefile) 
{
    for (ADDRINT addrbyte = addr; addrbyte < (addr + len); addrbyte++) 
    {
        if (writelog.find(addrbyte) == writelog.end()) 
        {
            UINT8 value = *(UINT8*)(addrbyte);
            writelog[addrbyte] = value;
            if (verbose) 
            {
                tracefile << "Saving Memory:\tLocation = " << hex << addrbyte
                          << "\tValue = " << (UINT32)value << "\n" << flush;
            }
        }
    }
}

VOID MEMLOG::RestoreMem(fstream& tracefile) 
{
    map<ADDRINT, UINT8>::const_iterator itr;
    for (itr = writelog.begin(); itr != writelog.end(); itr++) 
    {
        ADDRINT addrbyte = itr->first;
        UINT8 value = itr->second;
        if (verbose) 
        {
            tracefile << "Restoring Memory:\tLocation = " << hex << addrbyte
                      << "\tValue = " << (UINT32)value << "\n" << flush;
        }
        *(UINT8*)(addrbyte) = value;
    }
    Reset();
}

BOOL MEMLOG::DetectConflict(ADDRINT addr, UINT32 len, BOOL iswrite, fstream& tracefile) 
{
    for (ADDRINT addrbyte = addr; addrbyte < (addr + len); addrbyte++) 
    {
        if ((writelog.find(addrbyte) != writelog.end()) ||
            (iswrite && (readlog.find(addrbyte) != readlog.end())))
        {
            if (verbose) 
            {
                tracefile << "Detecting Conflict:\tLocation = " << hex << addrbyte << "\n" << flush;
            }
            return(true);
        }
    }
    return(false);
}

VOID MEMLOG::Reset() 
{
    readlog.clear();
    writelog.clear();
}

VOID MEMLOG::DumpMemState(fstream& dumpfile, fstream& tracefile, ADDRINT sp)
{
    map<ADDRINT, UINT8>::const_iterator itr;
    for (itr = writelog.begin(); itr != writelog.end(); itr++) 
    {
        ADDRINT addrbyte = itr->first;
        UINT8 value = *(UINT8*)(addrbyte);
        dumpfile << hex << addrbyte << "\t" << (UINT32)value << "\n" << flush;
        if (verbose) 
        {
            tracefile << "Dumping Memory:\tLocation = " << hex << addrbyte
                      << "\tValue = " << (UINT32)value << "\n" << flush;
        }
    }
    writelog.clear();
    dumpfile << hex << 0xffffffff << "\t" << 0 << "\n" << flush;
}

VOID MEMLOG::InitWriteLog(fstream& dumpfile, fstream& tracefile)
{
    ADDRINT addrbyte;
    UINT32 value;
    ASSERTX(writelog.empty());
    dumpfile >> hex >> addrbyte;
    dumpfile >> hex >> value;
    while(addrbyte != 0xffffffff) 
    {
        ASSERT(writelog.find(addrbyte) == writelog.end(), hexstr(addrbyte) + "\n");
        writelog[addrbyte] = (UINT8)value;
        if (verbose) 
        {
            tracefile << "Remembering Memory:\tLocation = " << hex << addrbyte
                      << "\tValue = " << value << "\n" << flush;
        }
        dumpfile >> hex >> addrbyte;
        dumpfile >> hex >> value;
    }
}

#endif  // #ifndef PINTOOL_MEMLOG_H
