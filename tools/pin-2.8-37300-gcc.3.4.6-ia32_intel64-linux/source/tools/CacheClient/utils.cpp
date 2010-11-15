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
//  This is a utility file that contains useful routines for 
//      all cache client tools

#include "pin.H"
#include "utils.H"
#include <iostream>

using namespace std;

/* ================================================================== */
/*
  Convert an unsigned integer (representing bytes) into a string 
  that uses KB or MB as necessary
*/
string BytesToString(UINT32 numBytes)
{
    if (numBytes < 10240)
        return decstr(numBytes) + " bytes"; 
    else if (numBytes < (1024*10240))
        return decstr(numBytes>>10) + " KB"; 
    else 
        return decstr(numBytes>>20) + " MB"; 
}

/* ================================================================== */
/*
  Print details of a cache initialization
*/
VOID PrintInitInfo()
{
    cout << "Cache Initialization Complete\t";

    UINT32 block_size = CODECACHE_BlockSize();
    UINT32 cache_limit = CODECACHE_CacheSizeLimit();
    
    if (cache_limit)
        cout << "[cache_limit=" << BytesToString(cache_limit) ;
    else 
        cout << "[cache_limit=unlimited" ;
    
    cout << ", cache_block=" << BytesToString(block_size) << "]" << endl;
}

/* ================================================================== */
/*
  Print command-line switches on error
*/
INT32 Usage()
{
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}
