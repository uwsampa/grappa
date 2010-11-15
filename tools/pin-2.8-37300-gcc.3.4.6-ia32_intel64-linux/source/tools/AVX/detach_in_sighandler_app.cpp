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
// @ORIGINAL_AUTHOR: Elena Demikhovsky

/*! @file
 *  This application detaches Pin in the middle of signal handler.
 * The test verifies signal frame after Pin detach
 */
#include <assert.h>
#include <stdio.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>
#include <ucontext.h>
#include <sched.h>
#include <sys/utsname.h>



using namespace std;


volatile bool sigHandled = false;

    struct YMM_REG
{
    unsigned int _a0;
    unsigned int _a1;
    unsigned int _a2;
    unsigned int _a3;
    unsigned int _a4;
    unsigned int _a5;
    unsigned int _a6;
    unsigned int _a7;
        
    YMM_REG() {}
        
    YMM_REG(int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7)
    :_a0(v0), _a1(v1), _a2(v2), _a3(v3), _a4(v4), _a5(v5), _a6(v6), _a7(v7)
    {}
        
    bool operator==(const YMM_REG& other) const
    {
        return (!memcmp(this, &other, sizeof(YMM_REG)));
    }
    bool operator!=(const YMM_REG& other) const
    {
        return (memcmp(this, &other, sizeof(YMM_REG)));
    }
    string hex() const
    {
        char buf[100];
        sprintf(buf, "0x%x_%x_%x_%x_%x_%x_%x_%x", _a7, _a6, _a5, _a4, _a3, _a2, _a1, _a0);
        string mystr(buf);
        return mystr;
    }
    void loadLower128(void *mem)
    {
        memcpy(&_a0, mem, 16);
    }
    void loadUpper128(void *mem)
    {
        memcpy(&_a4, mem, 16);
    }
    void storeLower128(void *mem) const
    {
        memcpy(mem, &_a0, 16);
    }
    void storeUpper128(void *mem) const
    {
        memcpy(mem, &_a4, 16);
    }
};

const YMM_REG ymm1_app(0x1111a, 0xa, 0x1111b, 0xb, 0x1111c, 0xc, 0x1111d, 0xd );
const YMM_REG ymm2_app(0x2222a, 0xa, 0x2222b, 0xb, 0x2222c, 0xc, 0x2222d, 0xd );
const YMM_REG ymm3_app(0x3333a, 0xa, 0x3333b, 0xb, 0x3333c, 0xc, 0x3333d, 0xd );


const YMM_REG ymm1_sig (0x81111a, 0xa8, 0x81111b, 0xb8, 0x81111c, 0xc8, 0x81111d, 0xd8 );
const YMM_REG ymm2_sig (0x82222a, 0xa8, 0x82222b, 0xb8, 0x82222c, 0xc8, 0x82222d, 0xd8 );
const YMM_REG ymm3_sig (0x83333a, 0xa8, 0x83333b, 0xb8, 0x83333c, 0xc8, 0x83333d, 0xd8 );

extern "C" void SetYmmRegs(const YMM_REG *v1, const YMM_REG *v2, const YMM_REG *v3);
extern "C" void GetYmmRegs(YMM_REG *v1, YMM_REG *v2, YMM_REG *v3);

struct _fpx_sw_bytes {
    unsigned int magic1;           /* FP_XSTATE_MAGIC1 (0x46505853) */
    unsigned int extended_size;    /* total size of the layout referred by
                                * fpstate pointer in the sigcontext.
                                * extended_size includes the
                                * size of FP_XSTATE_MAGIC2 (0x46505845), which
                                * will be present at the end of memory layout.
                            */
    unsigned int xstate_bv[2];
                                /* feature bit mask (including fp/sse/extended
                                * state) that is present in the memory
                                * layout. Essentially the feature bitmask
                                * enabled.
                                */
    unsigned int xstate_size;      /* actual xsave state size, based on the
                            * features saved in the layout (or in another
                            * words size of features enabled by OS).
                            * 'extended_size' will be greater than
                            * 'xstate_size'.
                            */
    unsigned int padding[7];       /*  for future use. */
};

struct xsave_hdr {
    unsigned long long xstate_bv;
    unsigned long long reserved1[2];
    unsigned long long reserved2[5];
};

#ifdef TARGET_IA32
struct fxsave
{
    unsigned short _fcw;
    unsigned short _fsw;
    unsigned char  _ftw;
    unsigned char  _pad1;
    unsigned short _fop;
    unsigned int _fpuip;
    unsigned short _cs;
    unsigned short _pad2;
    unsigned int _fpudp;
    unsigned short _ds;
    unsigned short _pad3;
    unsigned int _mxcsr;
    unsigned int _mxcsrmask;
    unsigned char  _st[8 * 16];
    unsigned char  _xmm[8 * 16];
    unsigned int  _reserved[44];
    struct _fpx_sw_bytes sw_reserved;
};


struct xstate
{
    struct xsave_hdr _xstate_hdr;
    unsigned char  _ymmUpper[8*16];
    unsigned char  _pad4[8*16];
};


struct KernelFpstate
{
    struct _libc_fpstate _fpregs_mem;   // user-visible FP register state (_mcontext points to this)
    struct fxsave _fxsave;           // full FP state as saved by fxsave instruction
    struct xstate _xstate;
};
#else
struct fxsave 
{
    unsigned short   _cwd;
    unsigned short   _swd;
    unsigned short   _twd;    /* Note this is not the same as the 32bit/x87/FSAVE twd */
    unsigned short   _fop;
    unsigned long    _rip;
    unsigned long    _rdp;
    unsigned int     _mxcsr;
    unsigned int     _mxcsrmask;
    unsigned int     _st[32];   /* 8*16 bytes for each FP-reg */
    unsigned char    _xmm[16 * 16];  /* 16*16 bytes for each XMM-reg  */
    unsigned int     _reserved2[12];
    struct _fpx_sw_bytes sw_reserved;
    
};

struct xstate
{
    struct xsave_hdr _xstate_hdr;
    unsigned char  _ymmUpper[16*16];
};


struct KernelFpstate
{
    struct fxsave _fxsave;   // user-visible FP register state (_mcontext points to this)
    struct xstate _xstate;
};

#endif

KernelFpstate *fpstatePtr = 0;


/* This function should be replaced by Pin tool */
extern "C" int ThreadHoldByPin()
{
    return 0;
}

/* This function should be replaced by Pin tool */
extern "C" void DetachPin()
{
    fprintf(stderr, "This function shouldn't be called form application\n");
    assert(false);
    return;
}

    
/*
 * A signal handler for canceling all threads
 */
void SigUsr1Handler(int signum, siginfo_t *siginfo, void *uctxt)
{    
    DetachPin();
    
    // Wait until Pin is detached 
    while (ThreadHoldByPin())
    {
        sched_yield();
    }
   
    ucontext_t *frameContext = (ucontext_t *)uctxt;
    
    /* Change application FP context */
    fpregset_t fpPtr = frameContext->uc_mcontext.fpregs;
    
    KernelFpstate *appFpState = reinterpret_cast < KernelFpstate * > (fpPtr);
    
    //printf("Attach debugger to %d\n", getpid());
    //getchar();
    
    YMM_REG appOrgYmm1;
    appOrgYmm1.loadLower128(appFpState->_fxsave._xmm+16);
    appOrgYmm1.loadUpper128(appFpState->_xstate._ymmUpper+16);
    
    YMM_REG appOrgYmm2;
    appOrgYmm2.loadLower128(appFpState->_fxsave._xmm+32);
    appOrgYmm2.loadUpper128(appFpState->_xstate._ymmUpper+32);
    
    YMM_REG appOrgYmm3;
    appOrgYmm3.loadLower128(appFpState->_fxsave._xmm+48);
    appOrgYmm3.loadUpper128(appFpState->_xstate._ymmUpper+48);
    
    
    if ((ymm1_app != appOrgYmm1) || (ymm2_app != appOrgYmm2) || (ymm3_app != appOrgYmm3))
    {
        cerr << "Unexpected ymm values after return from signal handler: " << hex << endl; 
        cerr << "ymm1 = " << appOrgYmm1.hex() << ", Expected " << ymm1_app.hex() << endl;
        cerr << "ymm2 = " << appOrgYmm2.hex() << ", Expected " << ymm2_app.hex() << endl;
        cerr << "ymm3 = " << appOrgYmm3.hex() << ", Expected " << ymm3_app.hex() << endl;
        exit(-1);
    }
    cout << "All ymm values in signal handler are correct" << endl;
    
    ymm1_sig.storeLower128(appFpState->_fxsave._xmm+16);
    ymm1_sig.storeUpper128(appFpState->_xstate._ymmUpper+16);
    
    ymm2_sig.storeLower128(appFpState->_fxsave._xmm+32);
    ymm2_sig.storeUpper128(appFpState->_xstate._ymmUpper+32);
    
    ymm3_sig.storeLower128(appFpState->_fxsave._xmm+48);
    ymm3_sig.storeUpper128(appFpState->_xstate._ymmUpper+48);
                
            
    sigHandled = true;
}

void SigUsr2Handler(int signum)
{    
    DetachPin();
    
    // Wait until Pin is detached
    while (ThreadHoldByPin())
    {
        sched_yield();
    }
 
    //printf("Attach debugger to %d\n", getpid());
    //getchar();
    
    sigHandled = true;
}

/*
 * Expected command line: <this exe> [-test NUM] 
 */

void ParseCommandLine(int argc, char *argv[], unsigned int *testNo)
{
    for (int i=1; i<argc; i++)
    {
        string arg = string(argv[i]);
        if (arg == "-test")
        {
            *testNo = atoi(argv[++i]);
        }
    }
}

/*
 * 2 tests: for default frame and for RT frame 
 */
 
int TestRtSigframe();
int TestSigframe();


int main(int argc, char *argv[])
{
    unsigned int testNo=0;
    ParseCommandLine(argc, argv, &testNo);
    
    if (testNo == 0)
    {
        return TestRtSigframe();
    }
    else
    {
        return TestSigframe();
    }
    return 0;
}

int TestRtSigframe()
{
    struct sigaction sSigaction;
    
    /* Register the signal hander using the siginfo interface*/
    sSigaction.sa_sigaction = SigUsr1Handler;
    sSigaction.sa_flags = SA_SIGINFO;
    
    /* mask all other signals */
    sigfillset(&sSigaction.sa_mask);
    
    int ret = sigaction(SIGUSR1, &sSigaction, NULL);
    if(ret) 
    {
        perror("ERROR, sigaction failed");
        exit(-1);
    }

    SetYmmRegs(&ymm1_app, &ymm2_app, &ymm3_app);
    kill(getpid(), SIGUSR1);
    
    while (!sigHandled)
    {
        sched_yield();
    }
    
    YMM_REG ymm1, ymm2, ymm3;
    GetYmmRegs(&ymm1, &ymm2, &ymm3);
    if ((ymm1_sig != ymm1) || (ymm2_sig != ymm2) || (ymm3_sig != ymm3))
    {
        cerr << "Unexpected ymm values after return from signal handler: " << hex << endl; 
        cerr << "ymm1 = " << ymm1.hex() << ", Expected " << ymm1_sig.hex() << endl;
        cerr << "ymm2 = " << ymm2.hex() << ", Expected " << ymm2_sig.hex() << endl;
        cerr << "ymm3 = " << ymm3.hex() << ", Expected " << ymm3_sig.hex() << endl;
        return -1;
    }
    cout << "All ymm values are correct" << endl;

    return 0;
}

int TestSigframe()
{
    signal(SIGUSR2, SigUsr2Handler);
    
    SetYmmRegs(&ymm1_app, &ymm2_app, &ymm3_app);
    kill(getpid(), SIGUSR2);
    
    while (!sigHandled)
    {
        sched_yield();
    }
    
    YMM_REG ymm1, ymm2, ymm3;
    GetYmmRegs(&ymm1, &ymm2, &ymm3);
    if ((ymm1_app != ymm1) || (ymm2_app != ymm2) || (ymm3_app != ymm3))
    {
        cerr << "Unexpected ymm values after return from signal handler: " << hex << endl; 
        cerr << "ymm1 = " << ymm1.hex() << ", Expected " << ymm1_app.hex() << endl;
        cerr << "ymm2 = " << ymm2.hex() << ", Expected " << ymm2_app.hex() << endl;
        cerr << "ymm3 = " << ymm3.hex() << ", Expected " << ymm3_app.hex() << endl;
        return -1;
    }
    
    cout << "All ymm values are correct" << endl;

    return 0;
}
