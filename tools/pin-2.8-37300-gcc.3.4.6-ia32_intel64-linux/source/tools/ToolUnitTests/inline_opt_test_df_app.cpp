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
#include <iostream>
#include <iomanip>
#include <cstdlib> // for atoi w/gcc4.3.x
using namespace std;
#define N 1024
int main(int argc, char** argv);
#if defined(_MSC_VER)
typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;
typedef __int8 INT8;
typedef __int16 INT16;
typedef __int32 INT32;
typedef __int64 INT64;
#define ALIGN16 __declspec(align(16))
#define ALIGN8  __declspec(align(8))

#else

#include <stdint.h>
typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint32_t UINT;
typedef uint64_t UINT64;

typedef int8_t  INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;
#define ALIGN16 __attribute__ ((aligned(16)))
#define ALIGN8  __attribute__ ((aligned(8)))


#endif




extern "C" void MyMemCpy(unsigned char *src, unsigned char *dst, int size);


int main(int argc, char** argv)
{
    unsigned char src[128];
    unsigned char dst[128];

    for (int i=0;i<128;i++)
    {
        src[i] = i;
        dst[i] = (i+1)&0xff;
    }
   
    for (int i=0;i<128;i++)
    {
        if (src[i] != i)
        {
            cout << "***Error in initialization of src" << endl;
            exit (-1);
        }
        if (dst[i] != ((i+1)&0xff))
        {
            cout << "***Error in initialization of dst" << endl;
            exit (-1);
        }
    }
    MyMemCpy (src, dst, 128); // MyMemCpy relied on the DF being 0
    for (int i=0;i<128;i++)
    {
        if (src[i] != i)
        {
            cout << "***Error in copy of src to dst, src changed" << endl;
            exit (-1);
        }
        if (dst[i] != i)
        {
            cout << "***Error in copy of src to dst, unexpected value in dst" << endl;
            exit (-1);
        }
    }
    return 0;
}
 
