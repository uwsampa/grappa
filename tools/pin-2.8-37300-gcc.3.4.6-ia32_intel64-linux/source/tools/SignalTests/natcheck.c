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
 * This test verifies that a signal handler gets the correct values for NaT bits
 * in the precise register context.  The DoNatTest() routine sets some NaT bits
 * and then waits for an alarm signal.  The signal handler makes sure the NaT
 * bits are correctly set in the context.  The handler also changes the context to
 * force some additional NaT bits to be set.  When DoNatTest() resumes, it checks
 * that all the expected NaT bits are set.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ucontext.h>

volatile unsigned SignalFlag = 0;

extern unsigned long DoNatTest();
static void Handler(int, siginfo_t *, void *);


int main()
{
    struct sigaction act;
    struct itimerval itval;
    unsigned long natBits;

    act.sa_sigaction = Handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGVTALRM, &act, 0) != 0)
    {
        fprintf(stderr, "Unable to set up VTALRM handler\n");
        return 1;
    }

    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_usec = 0;
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_usec = 500000;
    if (setitimer(ITIMER_VIRTUAL, &itval, 0) != 0)
    {
        fprintf(stderr, "Unable to set up timer\n");
        return 1;
    }

    natBits = DoNatTest();
    printf("NaT bits after test = 0x%lx\n", natBits);

    if (natBits != 0x4280)
    {
        fprintf(stderr, "Expected NaT's for R7, R9, R14, but got 0x%lx\n", natBits);
        return 1;
    }

    return 0;
}


static void Handler(int sig, siginfo_t *siginfo, void *vctxt)
{
    ucontext_t *ctxt = vctxt;

    printf("NaT bits in handler: 0x%lx\n", (unsigned long)ctxt->uc_mcontext.sc_nat);

    /*
     * Note, the real kernel only saves the register state of the "scratch" registers
     * in 'ctxt'.  It's safe to check for the R9 NaT bit because R9 is a scratch register.
     * Do not check for the NaT bit of a "saved" register here, though.
     */

    if (!(ctxt->uc_mcontext.sc_nat & 0x200))
    {
        fprintf(stderr, "R9 NaT is not set\n");
        exit(1);
    }

    /* Force the NaT bit of R14 */
    ctxt->uc_mcontext.sc_nat |= 0x4000;

    SignalFlag = 1;
}
