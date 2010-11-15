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
/*! @file
 *  An example of Windows application that raises exception and verifies
 *  the FP/XMM state. 
 */
#include <windows.h>
#include <string>
#include <iostream>
#include <memory.h>

using namespace std;

typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;

#ifdef TARGET_IA32
typedef UINT32 ADDRINT;
#else
typedef UINT64 ADDRINT;
#endif



#include "xsave_type.h"
#include "fp_context_manip.h"

#include "ymm_apps_stuff.h"


/*!
 * Exit with the specified error message
 */
static void Abort(const char * msg)
{
    cerr << msg << endl;
    exit(1);
}

/*!
 * Exception filter. 
 */
static int enteredFilter = 0;
static int MyExceptionFilter(LPEXCEPTION_POINTERS exceptPtr)
{
    enteredFilter = 1;
    if (!CheckMyFpContextInContext(exceptPtr->ContextRecord, 0))
    {
        Abort("Mismatch in the FP context");
    }
    return EXCEPTION_EXECUTE_HANDLER;

}




/*!
 * The main procedure of the application.
 */
int main(int argc, char *argv[])
{
     
    

    __try
    {
        char * p = 0;
        p++;
        SetMyFpContext(0);
        *p = 0;
    }
    __except (MyExceptionFilter(GetExceptionInformation()))
    {
    }
    
    if (!enteredFilter)
    {
        cerr << "FAILURE: MyExceptionFilter not entered" << endl;
    }
    cerr << "Success" << endl;
    return 0;
}
