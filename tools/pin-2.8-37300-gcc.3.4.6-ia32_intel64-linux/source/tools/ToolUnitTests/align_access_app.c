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
#include <stdlib.h>
#include <memory.h>

#if defined(TARGET_WINDOWS)
#define EXPORT_CSYM  __declspec( dllexport )
#else
#define EXPORT_CSYM 
#endif

typedef union {
    unsigned char *p;
    unsigned long l;
} PTOL;

EXPORT_CSYM unsigned long ptr2long(unsigned char *ptr)
{
    PTOL cast;
    cast.p = ptr;
    return cast.l;
}

EXPORT_CSYM void set_value(unsigned char *buf, unsigned int val)
{
    unsigned int *arr = (unsigned int *)buf;
    arr[0] = val;
}

EXPORT_CSYM int check_align(unsigned char *ptr)
{
    if (ptr2long(ptr) % 16)
        return 1;

    return 0;
}

int main()
{
    // align the buffer on 16 and call set value in a loop
    unsigned char buff[1000];
    unsigned char *ptr = buff + (16 - (ptr2long(buff) % 16));
    int i;

    memset(buff, 0, 1000);

    printf("Base pointer: %p\n", ptr);
    for (i = 0; i < 20; i++)
        set_value(ptr + 16 * i, 1);

    for (i = 0; i < 1000; i++)
    {
        if (buff[i] && check_align(&buff[i]))
        {
            printf("Align check failed on %p\n", &buff[i]);
            exit(1);
        }
    }

    printf("Align check ok\n");
    exit(0);
}
