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
// This tool prints a trace of image load and unload events
//

#include "pin.H"
#include <stdio.h>

using namespace std;

FILE * trace;

// Pin calls this function every time a new img is loaded
VOID ImageLoad(IMG img, VOID *v)
{
    SEC sec;
    RTN rtn;

    if ( string::npos == IMG_Name(img).find( "dummy_dll" ) ) 
        return;

    fprintf(trace, "library: %s\n", IMG_Name(img).c_str() );

    for ( sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) ) 
        for ( rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn) )
            fprintf(trace, "rtn: %s\n", RTN_Name(rtn).c_str() );

    fflush(trace);
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    trace = fopen("imageload.out", "w");

    // Initialize symbol processing
    PIN_InitSymbols();
    
    // Initialize pin
    if (PIN_Init(argc, argv) != 0)
    {
        return 1;
    }

    // Register ImageLoad to be called when an image is loaded
    IMG_AddInstrumentFunction(ImageLoad, 0);

    // Start the program, never returns
    if (PIN_IsProbeMode()) 
    {
        fprintf(trace, "Probe mode\n");
        PIN_StartProgramProbed();
    } else 
    {
        fprintf(trace, "JIT mode\n");
        PIN_StartProgram();
    }

    return 0;
}
