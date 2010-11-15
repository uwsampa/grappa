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
// This is the test application for tracing segment registers in FreeBSD
// I creates a new thread and run the function Print in it.
// This function executes a few function calls that eventually modify the 
// the thread-local-storage and create calls to the set/get system call
// and access via the TLS segment register.
//


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define TLS_SIZE 32

static void *Print(void *);

int main()
{
    pthread_t tid;

    pthread_create(&tid, 0, Print, 0);
    pthread_join(tid, 0);
    return 0;
}

static void *Print(void *arg)
{
    struct timespec req;
    long id = 0;
    void *fsbase = 0;
    void *newfsbase = 0;
    int i;
    
    printf("Hello world\n");

    printf("In thread_func(%p)\n", arg);

    // Get the system thread ID
    if (thr_self(&id) != 0)
    {
        fprintf(stderr, "Error calling thr_self: [%d] %s\n", errno, strerror(errno));
        goto end;
    }

    printf("Thread ID: %d\n", id);

    // Play around with new TLS area. Get the original TLS pointer
    if (amd64_get_fsbase(&fsbase) != 0)
    {
        fprintf(stderr, "Error calling amd64_get_fsbase: [%d] %s\n", errno, strerror(errno));
        goto end;
    }

    // Allocate a new TLS block. The size is taken from the sources of libthr.
    printf("FS base: %p\n", fsbase);
    newfsbase = malloc(TLS_SIZE);
    printf("New TLS base: %p\n", newfsbase);
    memcpy(newfsbase, fsbase, TLS_SIZE);
    *(long **)newfsbase = (long *)newfsbase;
    if (amd64_set_fsbase(newfsbase) != 0)
    {
        fprintf(stderr, "Error calling amd64_set_fsbase: [%d] %s\n", errno, strerror(errno));
        goto end;
    }

    // Call the nanosleep system call and read errno. Both write to TLS
    req.tv_sec = 0;
    req.tv_nsec = 100000;
    nanosleep(&req, 0);
    printf("Value of errno: %d\n", errno);

    // Restore original TLS
    printf("Restoring original TLS\n");
    if (amd64_set_fsbase(fsbase) != 0)
    {
        fprintf(stderr, "Error calling amd64_set_fsbase: [%d] %s\n", errno, strerror(errno));
        goto end;
    }

end:
    return 0;
}
