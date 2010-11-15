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
 *  The test verifies that FP state of thread is not changed by Pin injection
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <syscall.h>
#include <sched.h>


using namespace std;


struct ThreadLock
{
    unsigned long _tid;
};

extern "C" void InitLock(ThreadLock *lock);
extern "C" void GetLock(ThreadLock *lock, unsigned long tid);
extern "C" void ReleaseLock(ThreadLock *lock);

/* 
 * The total number of threads that should run in this process
 * The number may be changed in command line with -th_num
 */
unsigned int numOfSecondaryThreads = 4;

/*
 * Get thread Id
 */
pid_t GetTid()
{
     return syscall(__NR_gettid);
}

#include "ymm_apps_stuff.h"
#include "xsave_type.h"
#define FPSTATE_ALIGNMENT 64
#ifdef TARGET_IA32E
#define NUM_YMM_REG_BYTES 16*16
#else
#define NUM_YMM_REG_BYTES 8*16
#endif
 
extern "C" void ReadFpContext(FPSAVE *buf);
extern "C" void WriteFpContext(FPSAVE *buf);
extern "C" int SupportsAvx();

bool CompareFpContext(FPSAVE  *fpContextFromPin, FPSAVE *fpContextFromXsave, bool compareYmm)
{
    bool compareOk = true;
    
    if ( fpContextFromPin->_fcw != fpContextFromXsave->_fcw)
    {
        fprintf (stderr,"fcw ERROR\n");
        compareOk = false;
    }
    if ( fpContextFromPin->_fsw != fpContextFromXsave->_fsw)
    {
        fprintf (stderr,"_fsw ERROR\n");
        compareOk = false;
    }
    if ( fpContextFromPin->_ftw != fpContextFromXsave->_ftw)
    {
        fprintf (stderr,"_ftw ERROR\n");
        compareOk = false;
    }
    if ( fpContextFromPin->_fop != fpContextFromXsave->_fop)
    {
        fprintf (stderr,"_fop ERROR\n");
        compareOk = false;
    }
    if ( fpContextFromPin->_fpuip != fpContextFromXsave->_fpuip)
    {
        fprintf (stderr,"_fpuip ERROR\n");
        compareOk = false;
    }
    /* the _cs field seems to be changing randomly when running 32on64 linux
       needs further investigation to prove it is not a Pin bug */
    if ( fpContextFromPin->_cs != fpContextFromXsave->_cs)
    {
        fprintf (stderr,"_cs ERROR\n");
        compareOk = false;
    } 
    if ( fpContextFromPin->_fpudp != fpContextFromXsave->_fpudp)
    {
        fprintf (stderr,"_fpudp ERROR\n");
        compareOk = false;
    }
    /* the _ds field seems to be changing randomly when running 32on64 linux
       needs further investigation to prove it is not a Pin bug */
    if ( fpContextFromPin->_ds != fpContextFromXsave->_ds)
    {
        fprintf (stderr,"_ds ERROR\n");
        compareOk = false;
    }  
    if ( fpContextFromPin->_mxcsr != fpContextFromXsave->_mxcsr)
    {
        fprintf (stderr,"_mxcsr ERROR\n");
        compareOk = false;
    }
    if ( fpContextFromPin->_mxcsrmask != fpContextFromXsave->_mxcsrmask)
    {
        fprintf (stderr,"_mxcsrmask ERROR\n");
        compareOk = false;
    }
    int i;
    for (i=0; i< 8*16; i++)
    {
        if ( fpContextFromPin->_st[i] != fpContextFromXsave->_st[i])
        {
            fprintf (stderr,"_st[%d] ERROR\n", i);
            compareOk = false;
        }
    }
    for (i=0; 
#ifdef TARGET_IA32E
        i< 16*16;
#else
        i< 8*16; 
#endif
        i++)
    {
        if ( fpContextFromPin->_xmm[i] != fpContextFromXsave->_xmm[i])
        {
            fprintf (stderr,"_xmm[%d] ERROR\n", i);
            compareOk = false;
        }
    }

    if (compareYmm)
    {
    for (i=0; 
#ifdef TARGET_IA32E
        i< 16*16;
#else
        i< 8*16; 
#endif
        i++)
    {
        if ( fpContextFromPin->_ymmUpper[i] != fpContextFromXsave->_ymmUpper[i])
        {
            fprintf (stderr,"_ymmUpper[%d] ERROR\n", i);
            compareOk = false;
        }
    }
    }


    return (compareOk);
}


bool waitForPin = true;
volatile unsigned int secThreadStarted = 0;
ThreadLock mutex;

void * ThreadFunc(void * arg)
{
    
    unsigned char *buf1 = new unsigned char[sizeof (FPSAVE)+FPSTATE_ALIGNMENT];
    FPSAVE *fpstateBuf1 = 
        reinterpret_cast<FPSAVE *>
        (( reinterpret_cast<ADDRINT>(buf1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf1, 0, sizeof (FPSAVE));

    unsigned char *buf2 = new unsigned char[sizeof (FPSAVE)+FPSTATE_ALIGNMENT];
    FPSAVE *fpstateBuf2 = 
        reinterpret_cast<FPSAVE *>
        (( reinterpret_cast<ADDRINT>(buf2) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf2, 0, sizeof (FPSAVE));
    
    
    // Do not call any routine that can change FP state
    // between two reads:
    
    // This is the first read
    ReadFpContext(fpstateBuf1);
    
    GetLock(&mutex, (unsigned long)GetTid());
    ++secThreadStarted;
    ReleaseLock(&mutex);
    
    while (waitForPin)
    {
        sched_yield();
    }
    
    // This is the second read
    ReadFpContext(fpstateBuf2);
    
    unsigned long res;
    if (!CompareFpContext(fpstateBuf1, fpstateBuf2, true))
    {
        printf("Fp state was changed in thread %d\n", GetTid());
        res = 0;
    }
    else
    {
        res = 1;
    }
    delete [] buf1;
    delete [] buf2;
    return (void *)res;
}
    

#define DECSTR(buf, num) { buf = (char *)malloc(10); sprintf(buf, "%d", num); }

inline void PrintArguments(char **inArgv)
{
    fprintf(stderr, "Going to run: ");
    for(unsigned int i=0; inArgv[i] != 0; ++i)
    {
        fprintf(stderr, "%s ", inArgv[i]);
    }
    fprintf(stderr, "\n");
}


/* AttachAndInstrument()
 * a special routine that runs $PIN
 */
void AttachAndInstrument(list <string > * pinArgs)
{
    list <string >::iterator pinArgIt = pinArgs->begin();

    string pinBinary = *pinArgIt;
    pinArgIt++;

    pid_t parent_pid = getppid();


    char **inArgv = new char*[pinArgs->size()+10];

    unsigned int idx = 0;
    inArgv[idx++] = (char *)pinBinary.c_str(); 
    inArgv[idx++] = (char*)"-pid"; 
    inArgv[idx] = (char *)malloc(10);
    sprintf(inArgv[idx++], "%d", parent_pid);

    for (; pinArgIt != pinArgs->end(); pinArgIt++)
    {
        inArgv[idx++]= (char *)pinArgIt->c_str();
    }
    inArgv[idx] = 0;
    
    PrintArguments(inArgv);

    execvp(inArgv[0], inArgv);
    fprintf(stderr, "ERROR: execv %s failed\n", inArgv[0]);
    kill(parent_pid, 9);
    return; 
}



/*
 * Expected command line: <this exe> [-th_num NUM] -pin $PIN -pinarg <pin args > -t tool <tool args>
 */

void ParseCommandLine(int argc, char *argv[], list < string>* pinArgs)
{
    string pinBinary;
    for (int i=1; i<argc; i++)
    {
        string arg = string(argv[i]);
        if (arg == "-th_num")
        {
            numOfSecondaryThreads = atoi(argv[++i]) - 1;
        }
        else if (arg == "-pin")
        {
            pinBinary = argv[++i];
        }
        else if (arg == "-pinarg")
        {
            for (int parg = ++i; parg < argc; parg++)
            {
                pinArgs->push_back(string(argv[parg]));
                ++i;
            }
        }
    }
    if (pinBinary.empty())
    {
        fprintf(stderr, "-pin parameter should be specified\n");
    }
    else
    {
        pinArgs->push_front(pinBinary);
    }
}

pthread_t *thHandle;
// This function should be replaced by Pin tool.
extern "C" int ThreadsReady(unsigned int numOfThreads)
{
    assert(numOfThreads == numOfSecondaryThreads+1);
    return 0;
}

void SetYmmRegs(UINT32 val)
{
    ymm_reg_t ymm_regs[NUM_YMM_REGS];
    for (UINT32 i=0; i<NUM_YMM_REGS; i++)
    {
        write_ymm_reg(i, val);
    } 
}
    
int main(int argc, char *argv[])
{
    list <string> pinArgs;
    ParseCommandLine(argc, argv, &pinArgs);


    
    /*
     * Allocate 2 buffers for FP state. The first buffer is filled before attach.
     * The second is just after.
     * We should avoid any operation that change YMM registers between two reads
    */
    unsigned char *buf1 = new unsigned char[sizeof (FPSAVE)+FPSTATE_ALIGNMENT];
    FPSAVE *fpstateBuf1 =
        reinterpret_cast<FPSAVE *>
            (( reinterpret_cast<ADDRINT>(buf1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf1, 0, sizeof (FPSAVE));


    unsigned char *buf2 = new unsigned char[sizeof (FPSAVE)+FPSTATE_ALIGNMENT];
    FPSAVE *fpstateBuf2 =
    reinterpret_cast<FPSAVE *>
        (( reinterpret_cast<ADDRINT>(buf2) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset(fpstateBuf2, 0, sizeof (FPSAVE));
    
    
    // initialize a mutex that will be used by threads
    InitLock(&mutex);
    
    thHandle = new pthread_t[numOfSecondaryThreads];

    // start all secondary threads
    for (unsigned int i = 0; i < numOfSecondaryThreads; i++)
    {
        pthread_create(&thHandle[i], 0, ThreadFunc, (void *)i);
    }

    while (secThreadStarted < numOfSecondaryThreads)
    {
        sched_yield();
    }
    
    

    SetYmmRegs(0xa5a5a5a5);
    //WriteFpContext(fpstateBuf0);
    // === First read is here
    ReadFpContext(fpstateBuf1);
    
    // I assume that fork() does not touch xmm registers
    pid_t child = fork();

    if (child == 0)
    {
        AttachAndInstrument(&pinArgs);
    }
    
    // Give enough time for all threads to get started 
    while (!ThreadsReady(numOfSecondaryThreads+1))
    {
        sched_yield();
    }
    
    // === Second read is here
    ReadFpContext(fpstateBuf2);        
    
    // tell other threads that Pin is attached
    waitForPin = false;    
    
    // now all secondary threads should exit
    // the returned value is not 0 if FP state wasn't correctly set 
    // after Pin attach
    
    bool result = true;
    for (unsigned int i = 0; i < numOfSecondaryThreads; i++)
    {
        void * threadRetVal;
        pthread_join(thHandle[i], &threadRetVal);
        if (threadRetVal != (void *)1)
        {
            result = false;
        }
    }
    if (!result)
    {
        printf("ERROR: FP registers are changed after Pin attach\n");
        return -1;
    }

    // Check the main thread    

    if (!CompareFpContext(fpstateBuf1, fpstateBuf2, 1))
    {
        printf ("CompareFpContext mismatch in main\n");
        return -1;
    }

    

    for (int i=0; i<NUM_YMM_REG_BYTES; i++)
    {
        if ( fpstateBuf2->_xmm[i] != 0xa5 || 
             fpstateBuf2->_ymmUpper[i] != 0xa5)
        {
            printf ("unexpected ymmval\n");
            return -1;
        }
    }
    delete [] buf1;
    delete [] buf2;
    
    printf("SUCCESS: FP registers are preserved after Pin attach\n");
    return 0;
}

