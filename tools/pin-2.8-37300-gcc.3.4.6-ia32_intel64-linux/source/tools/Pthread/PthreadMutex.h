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
/* PthreadMutex:                                                               */
/* manipulates pthread mutex objects                                           */
/* manages the synchronization of threads using mutexes                        */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_MUTEX_H
#define PTHREAD_MUTEX_H

#include "PthreadMutexAttr.h"

namespace PinPthread 
{

    #define PTHREAD_MUTEXKIND_INVALID -1

    /* Note: we use (int)pthread_mutex_t::__m_reserved to store the (pthread_t)owner */

    class PthreadMutex 
    {
      public:
        static int pthread_mutex_destroy(pthread_mutex_t*);
        static int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
      public:
        static bool IsLocked(pthread_mutex_t*);
        static void Lock(pthread_mutex_t*, pthread_t);
        static void LockAgain(pthread_mutex_t*);
        static void Unlock(pthread_mutex_t*);
        static void UnlockAgain(pthread_mutex_t*);
        static pthread_t GetOwner(pthread_mutex_t*);
        static int GetKind(pthread_mutex_t*);
    };

    typedef std::vector<pthread_t> pthread_priorityqueue_t;

    class PthreadsSpinning
    {
      public:
        PthreadsSpinning(pthread_t);
        ~PthreadsSpinning();
        inline void PushThread(pthread_t);
        inline pthread_t PopThread();
        inline bool IsEmpty();
      public:
        pthread_priorityqueue_t spinning; // fifo of spinning threads for one mutex
    };

    typedef std::map<pthread_mutex_t*, PthreadsSpinning*> pthreadspinning_queue_t;

    class PthreadMutexManager 
    {
      public:
        int Lock(pthread_t, pthread_mutex_t*, bool);
        bool Unlock(pthread_t, pthread_mutex_t*, pthread_t*, int*);
      private:
        void BlockThread(pthread_t, pthread_mutex_t*);
        bool UnblockThread(pthread_mutex_t*, pthread_t*);
      private:
        pthreadspinning_queue_t spinning; // list of spinning threads indexed by mutex
    };
    
} // namespace PinPthread
    
#endif  // #ifndef PTHREAD_MUTEX_H
