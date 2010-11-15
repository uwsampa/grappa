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
 * This test verifies that Pin can attach to a process when one of the integer registers has a NaT
 * bit set.  The NatCheck() routine forces a NaT bit to be set and then delays for a short while.
 * In order for the test to be valid, Pin should attempt to attach during this delay.  After the
 * delay, the test verifies that the NaT bit is still set (i.e. it's still set when running jitted
 * within Pin).
 *
 * Currently, this test fails because of a bug in the kernel implementation of ptrace().  It appears
 * that the Pin process is not able to read the values of the NaT bits via ptrace() when attaching to
 * the application process.  The debugger appears to have a similar problem, as it is also not able
 * to correctly print the values of any NaT bits.  If this kernel bug is ever fixed, we should
 * re-enable this test.
 */

#include <stdio.h>
#include <unistd.h>

int main()
{
    fprintf(stderr, "Attach to me: %d\n", getpid());

    if (NatCheck())
    {
        printf("NaT bit correctly set\n");
        return 0;
    }
    else
    {
        printf("NaT bit incorrectly cleared!\n");
        return 1;
    }
}
