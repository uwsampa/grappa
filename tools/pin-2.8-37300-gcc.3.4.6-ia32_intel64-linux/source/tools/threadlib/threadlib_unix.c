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
 *  Implementation of the threading API in Unix. 
 */

#include "../threadlib/threadlib.h"
#include <sched.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>

BOOL CreateOneThread(THREAD_HANDLE * pThreadHandle, THREAD_RTN_PTR threadRtn, void * arg)
{
    pthread_t pthreadId;
    int rval;

    rval = pthread_create(&pthreadId, 0, threadRtn, arg);
    if (rval != 0) {
        perror("thread");
        fprintf(stdout, "pthread_create() failed with code: %d\n", rval);
        fflush(stdout);
        return FALSE;
    }

    *pThreadHandle = (THREAD_HANDLE)pthreadId;
    return TRUE;
}

BOOL JoinOneThread(THREAD_HANDLE threadHandle)
{
    pthread_t pthreadId = (pthread_t)threadHandle;
    pthread_join(pthreadId, 0);
    return TRUE;
}

void ExitCurrentThread()
{
    pthread_exit(0);
}

void DelayCurrentThread(unsigned int millisec)
{
#if defined(TARGET_LINUX)
    sched_yield();
#endif
    usleep(millisec*1000);
}
