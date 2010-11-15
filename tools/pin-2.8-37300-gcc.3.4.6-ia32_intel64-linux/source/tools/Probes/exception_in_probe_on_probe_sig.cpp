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
#include <stdio.h>
#include <string>

#include "pin.H"

namespace WND
{
#include <windows.h>
}

typedef int (* foo_t)();

static AFUNPTR foo_ptr1;

static int foo_rep1()
{
	printf("foo rep1 called\n");

	return ((foo_t)foo_ptr1)();
}

static int foo_rep2(foo_t orig_foo, ADDRINT returnIp)
{
	printf("foo rep2 called\n");

	int res = orig_foo();

    // May not be executed if exception occurs in previous statement
    printf("Caller IP = %X\n", returnIp);

    return res;
}

static VOID on_module_loading(IMG img, VOID *data)
{
    if (IMG_IsMainExecutable(img))
	{
		RTN routine = RTN_FindByName(img, "foo");
		if (!RTN_Valid(routine))
		{
            routine = RTN_FindByName(img, "_foo");
        }

		if (RTN_Valid(routine))
		{
			foo_ptr1 = RTN_ReplaceProbed(routine, (AFUNPTR)foo_rep1);

            PROTO foo_proto = PROTO_Allocate( PIN_PARG(int), CALLINGSTD_DEFAULT,
                                             "foo", PIN_PARG_END() );
            AFUNPTR foo_ptr2 = RTN_ReplaceSignatureProbed(routine, (AFUNPTR)foo_rep2,
                IARG_PROTOTYPE, foo_proto,
                IARG_ORIG_FUNCPTR,
                IARG_RETURN_IP,
                IARG_END);
            ASSERTX(foo_ptr2 != 0);
		}
	}
}

int main(int argc, char** argv)
{
    PIN_InitSymbols();

    if (!PIN_Init(argc, argv))
    {
        IMG_AddInstrumentFunction(on_module_loading,  0);        

        PIN_StartProbedProgram();
    }

    exit(1);
}
