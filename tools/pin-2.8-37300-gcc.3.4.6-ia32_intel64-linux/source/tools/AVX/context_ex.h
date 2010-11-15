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
/*  context_ex.h
 *  Extended context record for procesors that implement the XSAVE and XRSTOR 
 *  instructions. The CONTEXT_EX structure is described in 
 *  http://msdn.microsoft.com/en-us/library/dd405464(VS.85).aspx
 *  This file should be removed once the CONTEXT_EX definition will be released 
 *  as part of in the official Windows SDK.
 */

#ifndef _CONTEXT_EX_H_
#define _CONTEXT_EX_H_

#if !defined(CONTEXT_EX_LENGTH)

#if defined(_AMD64_)

#define CONTEXT_XSTATE (CONTEXT_AMD64 | 0x00000020L)
#define _MAX_YMM_REGISTER 15
#else
#define _MAX_YMM_REGISTER 7
#define CONTEXT_XSTATE (CONTEXT_i386 | 0x00000040L)


typedef struct _M128A {
    ULONG64	Low;
    ULONG64	High;	
} M128A;

#endif

#define SIZE_OF_FX_REGISTERS            128

typedef struct _YMMCONTEXT {
    M128A Ymm0;
    M128A Ymm1;
    M128A Ymm2;
    M128A Ymm3;
    M128A Ymm4;
    M128A Ymm5;
    M128A Ymm6;
    M128A Ymm7;
    M128A Ymm8;
    M128A Ymm9;
    M128A Ymm10;
    M128A Ymm11;
    M128A Ymm12;
    M128A Ymm13;
    M128A Ymm14;
    M128A Ymm15;
} YMMCONTEXT, *PYMMCONTEXT;


#pragma pack(1)

typedef struct _FXSAVE_FORMAT {
    USHORT  ControlWord;
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   MXCsrMask;
    UCHAR   RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT;


typedef  struct _AVX_IMAGE {
    FXSAVE_FORMAT   LegacyImage;
    ULONG64 XBV;
    ULONG64 Reserved[7];
    YMMCONTEXT YmmContext;    
} AVX_IMAGE, *PAVX_IMAGE;

typedef struct _XSTATE {
    ULONG64 XBV;
    ULONG64 Reserved[7];   
    YMMCONTEXT YmmContext;    
} XSTATE, *PXSTATE;
#pragma pack()

typedef struct _CONTEXT_CHUNK {
    LONG Offset;
    ULONG Length;
} CONTEXT_CHUNK, *PCONTEXT_CHUNK;



typedef struct _CONTEXT_EX {

    //
    // The total length of the structure starting from the chunk with 
    // the smallest offset. N.B. that the offset may be negative.
    //

    CONTEXT_CHUNK All;

    //
    // Wrapper for the traditional CONTEXT structure. N.B. the size of 
    // the chunk may be less than sizeof(CONTEXT) is some cases (when
    // CONTEXT_EXTENDED_REGISTERS is not set on x86 for instance).
    //

    CONTEXT_CHUNK Legacy;

    //
    // CONTEXT_XSTATE: Extended processor state chunk. The state is 
    // stored in the same format XSAVE operation strores it with 
    // exception of the first 512 bytes, i.e. staring from 
    // XSAVE_AREA_HEADER. The lower two bits corresponding FP and 
    // SSE state must be zero.
    //

    CONTEXT_CHUNK XState;

} CONTEXT_EX, *PCONTEXT_EX;

#endif /* CONTEXT_EX_LENGTH */

#if !defined(_MAX_YMM_REGISTER)

#if defined(_AMD64_)
#define _MAX_YMM_REGISTER 15
#else
#define _MAX_YMM_REGISTER 7
#endif

typedef struct _YMMCONTEXT {
    M128A Ymm0;
    M128A Ymm1;
    M128A Ymm2;
    M128A Ymm3;
    M128A Ymm4;
    M128A Ymm5;
    M128A Ymm6;
    M128A Ymm7;
    M128A Ymm8;
    M128A Ymm9;
    M128A Ymm10;
    M128A Ymm11;
    M128A Ymm12;
    M128A Ymm13;
    M128A Ymm14;
    M128A Ymm15;
} YMMCONTEXT, *PYMMCONTEXT;


#pragma pack(1)
typedef struct _XSTATE {
    ULONG64 XBV;
    ULONG64 Reserved[7];   
    YMMCONTEXT YmmContext;    
} XSTATE, *PXSTATE;
#pragma pack()

#endif /* _MAX_YMM_REGISTER */

#endif