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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sched.h>

/*
 * This program tests for a bug with waitpid. We run pin in a child and
 * wait for it. When pin uses ptrace attach to inject itself, the waitpid
 * fails with ECHILD. This appears to be a bug in waitpid that is fixed in
 * 2.6 kernels. Pin works around the problem by not using attach to inject
 * itself on 2.4 systems (except red hat el3 which is know to be ok).
 */

main(int argc, char * argv[])
{
    pid_t pid = fork();
    if (pid)
    {
        while(1)
        {
            pid_t cpid = waitpid(0, 0, WNOHANG);
            if (cpid > 0)
            {
                fprintf(stderr,"Child exited %d\n", cpid);
                exit(0);
            }
            
            if (cpid < 0)
            {
                perror("wait:");
                exit(1);
            }

            sched_yield();
        }
    }
    else
    {
        execl(argv[1], argv[1], "-xyzzy", "-pause_injector_parent", "2", "--", argv[2], 0);
        fprintf(stderr, "exec failed\n");
    }
}

        
