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
/* --------------------------------------------------------------------------- */
/* PthreadCancel:                                                              */
/* manages the cancellation state of all threads                               */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_CANCEL_H
#define PTHREAD_CANCEL_H

#include "PthreadUtil.h"

namespace PinPthread 
{
    class PthreadCancel 
    {
      public:
        PthreadCancel();
        ~PthreadCancel();
      public:        
        bool canceled;     // whether the thread has been canceled
        int cancelstate;   // whether thread can be canceled
        int canceltype;    // whether thread should honor cancellation immediately
    };

    typedef std::map<pthread_t, PthreadCancel*> pthreadcancel_queue_t;
    
    class PthreadCancelManager 
    {
      public:
        void AddThread(pthread_t);
        void KillThread(pthread_t);
        bool Cancel(pthread_t);
        bool IsCanceled(pthread_t);
        int  SetState(pthread_t, int, int*);
        int  SetType(pthread_t, int, int*);
      private:
        PthreadCancel* GetThreadCancelState(pthread_t);
      private:
        pthreadcancel_queue_t pthreads; // track cancellation state of each thread
    };
    
} // namespace PinPthread

#endif  // #ifndef PTHREAD_CANCEL_H
