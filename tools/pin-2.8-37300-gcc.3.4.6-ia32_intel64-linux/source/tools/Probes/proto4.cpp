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
  @ORIGINAL_AUTHOR: Vijay Janapa Reddi
  @MAJOR_REWRITE: Gail Lyons
*/

/* ===================================================================== */
/*! @file
  Replace an original function with a custom function defined in the tool. The
  new function can have either the same or different signature from that of its
  original function.  This tool works in both JIT mode and Probe mode.
*/

/* ===================================================================== */
#include "pin.H"
#include <iostream>
#include <stdlib.h>

using namespace std;

/* ===================================================================== */
/* Arrays needed for multiple replacements. */ 
/* ===================================================================== */

typedef VOID * (*FUNCPTR_MALLOC)(size_t);

VOID Sanity( IMG img, RTN rtn );
VOID ReplaceProbed( RTN rtn, PROTO proto_malloc );
VOID ReplaceJitted( RTN rtn, PROTO proto_malloc );
VOID * Probe_Malloc_IA32( FUNCPTR_MALLOC orgFuncptr, size_t arg0);
VOID * Jit_Malloc_IA32( CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0);
VOID * Probe_Malloc_IPF( FUNCPTR_MALLOC orgFuncptr, size_t arg0, ADDRINT appTP );
VOID * Jit_Malloc_IPF( FUNCPTR_MALLOC orgFuncptr, size_t arg0, ADDRINT appTP );
VOID PrintContext( CONTEXT * ctxt);



/* ===================================================================== */
VOID ImageLoad(IMG img, VOID *v)
{
    if ( ! IMG_IsMainExecutable(img) )
        return;
    
    
    cout << IMG_Name(img) << endl;
    
    PROTO proto_malloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                         "my_malloc", PIN_PARG(size_t), PIN_PARG_END() );
    
    RTN rtn = RTN_FindByName(img, "my_malloc");
    if (RTN_Valid(rtn))
    {
        Sanity(img, rtn);
        
        cout << "Replacing my_malloc in " << IMG_Name(img) << endl;
        
        if ( PIN_IsProbeMode())
            ReplaceProbed(rtn, proto_malloc);
        else
            ReplaceJitted(rtn, proto_malloc);
        
    }
    
    PROTO_Free( proto_malloc );
}

/* ===================================================================== */

int main(INT32 argc, CHAR *argv[])
{
    PIN_InitSymbols();

    PIN_Init(argc, argv);

    IMG_AddInstrumentFunction(ImageLoad, 0);
    
    if ( PIN_IsProbeMode() )
        PIN_StartProgramProbed();
    else
        PIN_StartProgram();
    
    return 0;
}


/* ===================================================================== */
/* Replacement routine support  */
/* ===================================================================== */


VOID Sanity(IMG img, RTN rtn)
{
    if ( PIN_IsProbeMode() && ! RTN_IsSafeForProbedReplacement( rtn ) )
    {
        cout << "Cannot replace " << RTN_Name(rtn) << " in " << IMG_Name(img) << endl;
        exit(1);
    }
}


/* ===================================================================== */

VOID ReplaceProbed( RTN rtn, PROTO proto_malloc)
{
#if defined ( TARGET_IA32 ) || defined ( TARGET_IA32E )

    RTN_ReplaceSignatureProbed(
        rtn, AFUNPTR( Probe_Malloc_IA32 ),
        IARG_PROTOTYPE, proto_malloc,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_END);
#else

    RTN_ReplaceSignatureProbed(
        rtn, AFUNPTR( Probe_Malloc_IPF ),
        IARG_PROTOTYPE, proto_malloc,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_REG_VALUE, REG_TP,
        IARG_END);
#endif
}


/* ===================================================================== */

VOID ReplaceJitted( RTN rtn, PROTO proto_malloc)
{
#if defined ( TARGET_IA32 ) || defined ( TARGET_IA32E )

    RTN_ReplaceSignature(
        rtn, AFUNPTR( Jit_Malloc_IA32 ),
        IARG_PROTOTYPE, proto_malloc,
        IARG_CONTEXT,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_END);
#else

    RTN_ReplaceSignature(
        rtn, AFUNPTR( Jit_Malloc_IPF ),
        IARG_PROTOTYPE, proto_malloc,
        IARG_ORIG_FUNCPTR,
        IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
        IARG_REG_VALUE, REG_TP,
        IARG_END);
#endif
}

/* ===================================================================== */


VOID * Probe_Malloc_IA32( FUNCPTR_MALLOC orgFuncptr, size_t arg0)
{
    VOID * v = orgFuncptr(arg0);
    return v;
}

/* ===================================================================== */

VOID * Jit_Malloc_IA32( CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0)
{
    VOID * ret;

    PIN_CallApplicationFunction( context, PIN_ThreadId(),
                                 CALLINGSTD_DEFAULT, orgFuncptr,
                                 PIN_PARG(void *), &ret,
                                 PIN_PARG(size_t), arg0,
                                 PIN_PARG_END() );

    return ret;
}

/* ===================================================================== */

VOID * Probe_Malloc_IPF( FUNCPTR_MALLOC orgFuncptr, size_t arg0, ADDRINT appTP )
{
    
#if defined (TARGET_IPF)
    IPF_SetTP( appTP );
#endif
    
    VOID * v = orgFuncptr(arg0);
    return v;
}

/* ===================================================================== */

VOID * Jit_Malloc_IPF( FUNCPTR_MALLOC orgFuncptr, size_t arg0, ADDRINT appTP )
{
    
#if defined (TARGET_IPF)
    IPF_SetTP( appTP );
#endif
    
    VOID * v = orgFuncptr(arg0);
    return v;
}


/* ===================================================================== */
/* eof */
/* ===================================================================== */
