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
/*
 * This test verifies that the CONTROL call-back for a thread happens after the thread-start call-back
 * for that thread.
 */

#include <stdlib.h>
#include "pin.H"
#include "instlib.H"

using namespace INSTLIB;


static VOID OnNewThread(THREADID, CONTEXT *, INT32, VOID *);
static VOID Handler(CONTROL_EVENT, VOID *, CONTEXT *, VOID *, THREADID);


CONTROL Control;


int main(int argc, char **argv)
{
    if (PIN_Init(argc, argv))
        return 1;

    PIN_AddThreadStartFunction(OnNewThread, 0);
    Control.CheckKnobs(Handler, 0);

    PIN_StartProgram();
}


static BOOL SeenThreads[PIN_MAX_THREADS];


static VOID OnNewThread(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    if (tid < PIN_MAX_THREADS)
        SeenThreads[tid] = TRUE;
}


static VOID Handler(CONTROL_EVENT ev, VOID *val, CONTEXT *ctxt, VOID *ip, THREADID tid)
{
    if (tid < PIN_MAX_THREADS && !SeenThreads[tid])
    {
        exit(1);
    }
}
