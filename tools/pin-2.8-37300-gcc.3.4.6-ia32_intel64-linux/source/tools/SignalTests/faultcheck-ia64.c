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
#include <signal.h>
#include <stdio.h>
#include <bits/sigcontext.h>
#include <sys/prctl.h>
#include "faultcheck-target.h"

extern void DoUnalign();
extern void DoIllegal();
extern void DoDiv0();
extern void DoInexact();
extern void DoBreak();


int Initialize()
{
    return 1;
}


TSTATUS DoTest(unsigned int tnum)
{
    unsigned int base;

    switch (tnum)
    {
    case 0:
        printf("  SEGV\n");
        *(int *)0x8 = 0;
        return TSTATUS_NOFAULT;
    case 1:
        printf("  Unalign\n");
        if (prctl(PR_SET_UNALIGN, PR_UNALIGN_SIGBUS) != 0)
            printf("Can't set unaligned\n");
        DoUnalign();
        return TSTATUS_NOFAULT;
    case 2:
        printf("  Illegal\n");
        DoIllegal();
        return TSTATUS_NOFAULT;
    case 3:
        printf("  Div0\n");
        DoDiv0();
        return TSTATUS_NOFAULT;
    case 4:
        printf("  Inexact\n");
        DoInexact();
        return TSTATUS_NOFAULT;
    case 5:
        printf("  Break\n");
        DoBreak();
        return TSTATUS_NOFAULT;
    }

    return TSTATUS_DONE;
}


void PrintSignalContext(int sig, const siginfo_t *info, void *vctxt)
{
    struct sigcontext *ctxt = vctxt;
    printf("  Signal %d, pc=0x%lx, si_addr=0x%lx, si_code=%d, si_errno=%d\n", sig,
        (unsigned long)ctxt->sc_ip,
        (unsigned long)info->si_addr,
        (int)info->si_code,
        (int)info->si_errno);
}
