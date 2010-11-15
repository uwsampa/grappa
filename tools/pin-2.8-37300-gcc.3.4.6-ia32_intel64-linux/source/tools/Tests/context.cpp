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
#include "pin.H"
#include <iostream>

UINT64 icount = 0;

LOCALFUN VOID PrintContext(CONTEXT * ctxt)
{
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
    cout << "gax:   " << ctxt->_gax << endl;
    cout << "gbx:   " << ctxt->_gbx << endl;
    cout << "gcx:   " << ctxt->_gcx << endl;
    cout << "gdx:   " << ctxt->_gdx << endl;
    cout << "gsi:   " << ctxt->_gsi << endl;
    cout << "gdi:   " << ctxt->_gdi << endl;
    cout << "gbp:   " << ctxt->_gbp << endl;
    cout << "sp:    " << ctxt->_stack_ptr << endl;

#if defined(TARGET_IA32E)
    cout << "r8:    " << ctxt->_r8 << endl;
    cout << "r9:    " << ctxt->_r9 << endl;
    cout << "r10:   " << ctxt->_r10 << endl;
    cout << "r11:   " << ctxt->_r11 << endl;
    cout << "r12:   " << ctxt->_r12 << endl;
    cout << "r13:   " << ctxt->_r13 << endl;
    cout << "r14:   " << ctxt->_r14 << endl;
    cout << "r15:   " << ctxt->_r15 << endl;
#endif
    
    cout << "ss:    " << ctxt->_ss << endl;
    cout << "cs:    " << ctxt->_cs << endl;
    cout << "ds:    " << ctxt->_ds << endl;
    cout << "es:    " << ctxt->_es << endl;
    cout << "fs:    " << ctxt->_fs << endl;
    cout << "gs:    " << ctxt->_gs << endl;
    cout << "gflags:" << ctxt->_gflags << endl;

    cout << "mxcsr: " << ctxt->_fxsave._mxcsr << endl;
    
#endif
}


VOID ShowContext(VOID * ip, VOID * handle, ADDRINT gax)
{
    CONTEXT ctxt;
    
    // Capture the context. This must be done first before some floating point
    // registers have been overwritten
    PIN_MakeContext(handle, &ctxt);

    static bool first = false;

    if (first)
    {
        cout << "ip:    " << ip << endl;
    
        PrintContext(&ctxt);

        cout << endl;
    }

#if defined(TARGET_IA32) || defined(TARGET_IA32E)
    ASSERTX(gax == ctxt._gax);
#endif    
}

VOID Trace(TRACE tr, VOID *v)
{
    TRACE_InsertCall(tr, IPOINT_BEFORE, AFUNPTR(ShowContext), IARG_INST_PTR, IARG_CONTEXT, IARG_REG_VALUE, REG_GAX, IARG_END);
}

int main(int argc, char * argv[])
{
    cout << hex;
    cout.setf(ios::showbase);
    
    PIN_Init(argc, argv);

    TRACE_AddInstrumentFunction(Trace, 0);
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}
