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
#if defined(TARGET_IA32)

struct FXSAVE_STRUCT
{
    UINT16 _fcw;
    UINT16 _fsw;
    UINT8  _ftw;
    UINT8  _pad1;
    UINT16 _fop;
    UINT32 _fpuip;
    UINT16 _cs;
    UINT16 _pad2;
    UINT32 _fpudp;
    UINT16 _ds;
    UINT16 _pad3;
    UINT32 _mxcsr;
    UINT32 _mxcsrmask;
    UINT8  _st[8 * 16];
    UINT8  _xmm[8 * 16];
    UINT8  _pad4[56 * 4];
};

struct FPSTATE_STRUCT
{
    // fxsave_legacy is applicable on all IA-32 and Intel(R) 64
    // processors
    struct FXSAVE_STRUCT fxsave_legacy;
    // the following are only applicable on processors with AVX
    UINT8  _extendedHeader[64];
    UINT8  _ymmUpper[8*16];
    UINT8  _pad5[8*16];
};
typedef FPSTATE_STRUCT FPSTATE;
typedef FXSAVE_STRUCT FXSAVE;


const size_t FpRegsOffset = offsetof(CONTEXT, FloatSave.RegisterArea);


const size_t XmmRegsOffset = (offsetof(CONTEXT, ExtendedRegisters) + offsetof(FPSTATE, fxsave_legacy._xmm[0]));


#elif defined(TARGET_IA32E)

struct FXSAVE_STRUCT
{
    UINT16 _fcw;
    UINT16 _fsw;
    UINT8  _ftw;
    UINT8  _pad1;
    UINT16 _fop;
    UINT32 _fpuip;
    UINT16 _cs;
    UINT16 _pad2;
    UINT32 _fpudp;
    UINT16 _ds;
    UINT16 _pad3;
    UINT32 _mxcsr;
    UINT32 _mxcsrmask;
    UINT8  _st[8 * 16];
    UINT8  _xmm[16 * 16];
    UINT8  _pad4[24 * 4];
};

struct FPSTATE_STRUCT
{
    // fxsave_legacy is applicable on all IA-32 and Intel(R) 64
    // processors
    struct FXSAVE_STRUCT fxsave_legacy;
    // the following are only applicable on processors with AVX
    UINT8  _extendedHeader[64];
    UINT8  _ymmUpper[16*16];
};
typedef FPSTATE_STRUCT FPSTATE;
typedef FXSAVE_STRUCT FXSAVE;

const size_t FpRegsOffset = offsetof(CONTEXT, FltSave.FloatRegisters);



const size_t XmmRegsOffset = offsetof(CONTEXT, FltSave.XmmRegisters);



#endif