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
#include <assert.h>
#include <stdio.h>
#include "../threadlib/threadlib.h"

int a[100000];
int n = 10;

void * hello(void * arg)
{
    int i,j;
    
    //printf("In child %d: %p \n", getpid(), arg);
    //fflush(stdout);
    //kill(0,SIGSTOP);
    
    for (j = 0; j < 1000; j++)
    {
        for (i = 0; i < n; i++)
        {
            a[i] = 1;
        }
    }

    return 0;
}

THREAD_HANDLE threads[MAXTHREADS];

int threads_started;

int main(int argc, char *argv[])
{
    int numthreads = 5;
    int numreps = 1000;
    int success;
    
    int i,j;
    
    //sscanf(argv[1], "%d", &numthreads);
    printf("Threads: %d * reps: %d\n", numthreads, numreps);
    
    assert(numthreads < MAXTHREADS);
    
    for (j = 0; j < numreps; j++)
    {
        
        for (i = 0; i < numthreads; i++)
        {
            //printf("Creating thread\n");
            //fflush(stdout);
            if (!CreateOneThread(&threads[i], hello, 0))
            {
                fprintf(stdout, "CreateOneThread failed\n");
            }
            fflush(stdout);
        }
        for (i = 0; i < numthreads; i++)
        {
            BOOL success;
            success = JoinOneThread(threads[i]);
            if (!success)
            {
                fprintf(stdout, "JoinOneThread failed\n");
                fflush (stdout);
            }
        }

        printf("Finished rep %d\n", j);
        fflush (stdout);
    }
    
    printf("All threads joined\n");
    fflush (stdout);

    return 0;
}
