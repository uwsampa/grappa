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
/*! @file
 * Insert a call before/after pthread_spin_lock() in probe mode.
 * Linux IA-32/Intel 64 architectures only.
 */

/* ===================================================================== */
#include "pin.H"
#include <cstdlib>
#include <iostream>

using namespace std;

/* ===================================================================== */
/* Analysis routines  */
/* ===================================================================== */

VOID Before( ADDRINT * lock )
{
    cout << "Before: lock = " << *lock << endl << flush;
}


VOID After( ADDRINT retval )
{
    cout << "After: return value = " << retval << endl << flush;
}


/* ===================================================================== */
/* Instrumentation routines  */

/* ===================================================================== */
VOID ImageLoad(IMG img, VOID *v)
{
    RTN rtn = RTN_FindByName(img, "pthread_spin_lock");

    if ( RTN_Valid(rtn) && RTN_IsSafeForProbedReplacementEx(rtn, PROBE_MODE_ALLOW_RELOCATION) )
    {
        PROTO proto = PROTO_Allocate( PIN_PARG(int), CALLINGSTD_DEFAULT,
                                      "pthread_spin_lock", PIN_PARG(void *),
                                      PIN_PARG_END() );
        
        RTN_InsertCallProbedEx(
            rtn, IPOINT_BEFORE, 
            PROBE_MODE_ALLOW_RELOCATION, AFUNPTR( Before ),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);

        RTN_InsertCallProbedEx(
            rtn, IPOINT_AFTER, 
            PROBE_MODE_ALLOW_RELOCATION, AFUNPTR( After ),
            IARG_PROTOTYPE, proto,
            IARG_REG_VALUE, REG_GAX,
            IARG_END);

        PROTO_Free( proto );
    }
}


/* ===================================================================== */

int main(INT32 argc, CHAR *argv[])
{
    PIN_InitSymbols();
    
    PIN_Init(argc, argv);
    
    IMG_AddInstrumentFunction(ImageLoad, 0);
    
    PIN_StartProgramProbed();
    
    return 0;
}



/* ===================================================================== */
/* eof */
/* ===================================================================== */
