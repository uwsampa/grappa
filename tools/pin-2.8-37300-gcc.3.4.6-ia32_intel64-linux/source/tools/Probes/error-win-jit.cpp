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
// This tool demonstrates how to get the value of the application's
// error code on windows in jit mode.
//

#include "pin.H"
#include <iostream>
#include <stdlib.h>
#include <errno.h>

using namespace std;

namespace WINDOWS
{
    #include <windows.h>
}

AFUNPTR pfnGetLastError = 0;


/* ===================================================================== */
VOID ToolCheckError(  CONTEXT * ctxt )
{
    unsigned long err_code;
    
    if ( pfnGetLastError != 0 )
    {
        cerr << "Tool: calling GetLastError()" << endl;
        
        PIN_CallApplicationFunction( ctxt, PIN_ThreadId(), CALLINGSTD_DEFAULT,
                                     pfnGetLastError, PIN_PARG(unsigned long), &err_code,
                                     PIN_PARG_END() );

        cerr << "Tool: error code=" << err_code << endl;
    }
    else
        cerr << "Tool: GetLastError() not found." << endl;
    
}

/* ===================================================================== */
VOID ImageLoad(IMG img, VOID *v)
{
    if ( IMG_IsMainExecutable( img ))
    {
        PROTO proto = PROTO_Allocate( PIN_PARG(void), CALLINGSTD_DEFAULT,
                                      "CheckError", PIN_PARG_END() );
        
        RTN rtn = RTN_FindByName(img, "CheckError");
        if (RTN_Valid(rtn))
        {
            cout << "Replacing " << RTN_Name(rtn) << " in " << IMG_Name(img) << endl;
            
            RTN_ReplaceSignature(rtn, AFUNPTR(ToolCheckError),
                                 IARG_PROTOTYPE, proto,
                                 IARG_CONTEXT,
                                 IARG_END);
            
        }    
        PROTO_Free( proto );
    }
}

/* ===================================================================== */
int main(INT32 argc, CHAR *argv[])
{
    pfnGetLastError = (AFUNPTR)WINDOWS::GetProcAddress(
                               WINDOWS::GetModuleHandle("kernel32.dll"), "GetLastError");

    PIN_InitSymbols();

    PIN_Init(argc, argv);

    IMG_AddInstrumentFunction(ImageLoad, 0);
    
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */


