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
 *  This file tests error reporting.
*/

/* ===================================================================== */
#include "pin.H"
#include <iostream>

using namespace std;

/* ===================================================================== */

/* ===================================================================== */
void * Malloc(  CONTEXT * ctxt, AFUNPTR pf_malloc, size_t size )
{
    void * res;

    // PIN_CallApplicationFunction() cannot be called in Probe mode!
    // This should result in an error.  Do not try this at home.
    //
    PIN_CallApplicationFunction( ctxt, PIN_ThreadId(),
                                 CALLINGSTD_DEFAULT, pf_malloc,
                                 PIN_PARG(int), &res,
                                 PIN_PARG(size_t), size,
                                 PIN_PARG_END() );
    

    return res;
}


/* ===================================================================== */
VOID ImageLoad(IMG img, VOID *v)
{
    
    PROTO proto = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                  "malloc", PIN_PARG(size_t),
                                  PIN_PARG_END() );
    
    RTN rtn = RTN_FindByName(img, "malloc");
    if (RTN_Valid(rtn))
    {
        RTN_ReplaceSignatureProbed(
            rtn, AFUNPTR(Malloc),
            IARG_PROTOTYPE, proto,
            IARG_CONTEXT,
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);

    }    
    PROTO_Free( proto );
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

