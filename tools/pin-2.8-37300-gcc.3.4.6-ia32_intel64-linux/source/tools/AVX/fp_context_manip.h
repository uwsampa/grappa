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
#include "context_ex.h"
#if defined(TARGET_IA32)

const size_t FpRegSize = 16;
const size_t NumFpRegs = 8;

const size_t XmmRegSize = 16;
const size_t NumXmmRegs = 8;

const size_t YmmRegUpperSize = 16;
const size_t NumYmmRegs = 8;

typedef UINT32 ADDRINTX;

#elif defined(TARGET_IA32E)

const size_t FpRegSize = 16;
const size_t NumFpRegs = 8;

const size_t XmmRegSize = 16;
const size_t NumXmmRegs = 16;

const size_t YmmRegUpperSize = 16;
const size_t NumYmmRegs = 16;

typedef UINT64 ADDRINTX;

#endif
#define FPSTATE_ALIGNMENT 64

extern "C" void Do_Xsave(FPSTATE *xsaveContext);
extern "C" void Do_Fxsave(FPSTATE *xsaveContext);
extern "C" void Do_Xrstor(FPSTATE *xsaveContext);
extern "C" void Do_Fxrstor(FPSTATE *xsaveContext);

/*!
 * Set some constant values in FP and XMM and upper part of YMM registers. 
 */
void SetMyFpContext(int valToAdd)
{
	unsigned char *buf1 = new unsigned char[sizeof (FPSTATE)+FPSTATE_ALIGNMENT];
    FPSTATE *fpstateBuf1 = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINTX>(buf1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf1, 0, sizeof (FPSTATE));
   
    Do_Xsave (fpstateBuf1);
	

	
    memset(&(fpstateBuf1->fxsave_legacy._st), 0, sizeof (fpstateBuf1->fxsave_legacy._st));
    for (size_t i = 0; i <NumFpRegs; ++i)
    {
        *((unsigned char *)(&(fpstateBuf1->fxsave_legacy._st)) + (i * FpRegSize)) = (unsigned char)i+valToAdd; 
    }

    memset(&(fpstateBuf1->fxsave_legacy._xmm), 0, sizeof (fpstateBuf1->fxsave_legacy._xmm));
    for (size_t i = 0; i < NumXmmRegs; ++i)
    {
        *((unsigned char *)(&(fpstateBuf1->fxsave_legacy._xmm)) + (i * XmmRegSize)) = (unsigned char)i+valToAdd; 
    }
	

    memset(&(fpstateBuf1->_ymmUpper), 0, sizeof (fpstateBuf1->_ymmUpper));
    for (size_t i = 0; i < NumYmmRegs; ++i)
    {
        *((unsigned char *)(&(fpstateBuf1->_ymmUpper)) + (i * YmmRegUpperSize)) = (unsigned char)i+valToAdd;
    }
	
	fpstateBuf1->_extendedHeader[0] = 7;
    Do_Xrstor (fpstateBuf1);
	
}






/*!
 * Check to see that values of FP/XMM/YMM registers in the specified context match
 * values assigned by SetMyFpContext().
 */
static bool CheckMyFpContextInContext(CONTEXT *context, int valToAdd)
{
	
    FPSTATE *fpSave;
#if defined(TARGET_IA32)
	fpSave = reinterpret_cast<FPSTATE *>(context->ExtendedRegisters);
#else
	fpSave = reinterpret_cast<FPSTATE *>(&(context->FltSave));
#endif
	//printf ("\nCheckMyFpContextInContext valToAdd %d\n", valToAdd);
	//printf ("\nst[]\n");
	for (size_t i = 0; i < sizeof (fpSave->fxsave_legacy._st); ++i)
    {
        unsigned char regId = i/FpRegSize + valToAdd;
		//printf ("  st[%d][%d] %x\n", i/FpRegSize, i%FpRegSize, (fpSave->fxsave_legacy._st[i])&0xff);

        if ((i%FpRegSize == 0) && 
            (*((unsigned char *)(&(fpSave->fxsave_legacy._st)) +  i) != regId))
        {
			printf ("false 1 at i %d\n", i);
            return false;
        }
        if ((i%FpRegSize != 0) && 
            (*((unsigned char *)(&(fpSave->fxsave_legacy._st)) + i) != 0))
        {
			printf ("false 2 at i %d\n", i);
            return false;
        }
    }

	//printf ("\nxmm[]\n");
    for (size_t i = 0; i < sizeof(fpSave->fxsave_legacy._xmm); ++i)
    {
        unsigned char regId = i/XmmRegSize + valToAdd;
		//printf ("  xmm[%d][%d] %x\n", i/FpRegSize, i%FpRegSize, (fpSave->fxsave_legacy._xmm[i])&0xff);

        if ((i%XmmRegSize == 0) && 
            (*((unsigned char *)(fpSave->fxsave_legacy._xmm) +  i) != regId))
        {
			printf ("false 3 at i %d\n", i);
            return false;
        }
        if ((i%XmmRegSize != 0) && 
            (*((unsigned char *)(fpSave->fxsave_legacy._xmm) + i) != 0))
        {
			printf ("false 4 at i %d\n", i);
            return false;
        }
    }
	

	PCONTEXT_EX extendedContext =  ( PCONTEXT_EX)(context+1);
    PXSTATE xStateInfo = reinterpret_cast< PXSTATE>((reinterpret_cast<ADDRINTX>(extendedContext)) + extendedContext->XState.Offset);
	unsigned char *ymmUpper = reinterpret_cast<unsigned char *>(&(xStateInfo->YmmContext));
    
	size_t upperLimit;
#ifdef TARGET_IA32
	upperLimit = sizeof(xStateInfo->YmmContext)/2;
#else
	upperLimit = sizeof(xStateInfo->YmmContext);
#endif
	//printf ("\nymm[]\n");
    for (size_t i = 0; i < upperLimit; ++i)
    {
        unsigned char regId = i/YmmRegUpperSize + valToAdd;
		//printf ("  ymmU[%d][%d] %x\n", i/FpRegSize, i%FpRegSize, (ymmUpper[i])&0xff);

        if ((i%YmmRegUpperSize == 0) && 
            (*((unsigned char *)(ymmUpper) + i) != regId))
        {
			printf ("false 5 at i %d\n", i);
            return false;
        }
        if ((i%YmmRegUpperSize != 0) && 
            (*((unsigned char *)(ymmUpper) +  i) != 0))
        {
			printf ("false 6 at i %d\n", i);
            return false;
        }
    }
	
    return true;
}