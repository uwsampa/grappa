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
extern "C" {

void marker_start_counting() { volatile int x =1; }
void marker_stop_counting() {volatile int x =1; }
void marker_emit_stats() {volatile int x =1; }
void marker_zero_stats() {volatile int x =1; }
}
#include <stdlib.h>
#include <stdio.h>
int main() {
    double a[100],b[100],c[100],d[100],e[100],y[100],z[100];
    int s = 0;
    int i;

    for(i=0;i<100;i++) {
        a[i] = 1.0*rand();
        b[i] = 1.0*rand();
        z[i] = 1.0*rand();
        y[i] = 1.0*rand();
    }
    marker_zero_stats();
    marker_start_counting();
    for(i=0;i<100;i++) {
        c[i] = a[i] + b[i];
    }
    for(i=0;i<90;i++) {
        d[i] = a[i] * b[i];
    }
    for(i=0;i<80;i++) {
        e[i] = a[i] * y[i] + z[i];
    }
    marker_stop_counting();    
    marker_emit_stats();
    for(i=0;i<100;i++) {
        s += (int)e[i]+(int)d[i]+(int)c[i];
    }
    printf("%d\n",s);
    return 0;
}
