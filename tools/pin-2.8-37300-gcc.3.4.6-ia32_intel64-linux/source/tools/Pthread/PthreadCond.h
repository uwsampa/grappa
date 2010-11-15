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
/* PthreadCond:                                                                */
/* manipulates pthread condition variables                                     */
/* manages the synchronization of threads using condition variables            */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_CONDATTR_H
#define PTHREAD_CONDATTR_H

#include "PthreadMutex.h"

namespace PinPthread
{

    class PthreadCond
    {
      public:
        static int pthread_cond_init(pthread_cond_t*, pthread_condattr_t*);
    };

    typedef std::pair<pthread_t, pthread_mutex_t*> pthread_waitstate_t;
    typedef std::vector<pthread_waitstate_t>       pthread_waiterfifo_t;
    
    class PthreadWaiters
    {
      public:
        PthreadWaiters(pthread_t, pthread_mutex_t*);
        ~PthreadWaiters();
        void PushWaiter(pthread_t, pthread_mutex_t*);
        void PopWaiter(pthread_t*, pthread_mutex_t**);
        bool IsEmpty();
      public:
        pthread_waiterfifo_t waiters; // fifo of waiting threads for one cond
    };
        
    typedef std::map<pthread_cond_t*, PthreadWaiters*> pthreadcond_queue_t;

    class PthreadCondManager 
    {
      public:
        bool HasMoreWaiters(pthread_cond_t*);
        void AddWaiter(pthread_cond_t*, pthread_t, pthread_mutex_t*);
        void RemoveWaiter(pthread_cond_t*, pthread_t*, pthread_mutex_t**);
      private:
        pthreadcond_queue_t waiters;   // list of waiting threads indexed by cond
    };
    
} // namespace PinPthread

#endif  // #ifndef PTHREAD_CONDATTR_H
