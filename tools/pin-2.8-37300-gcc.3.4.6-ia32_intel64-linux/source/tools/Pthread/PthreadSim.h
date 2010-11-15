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
/* PthreadSim:                                                                 */
/* custom pthread library implementation                                       */
/* provides the standard pthread API and context switching mechanism           */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_SIM_H
#define PTHREAD_SIM_H

#include "PthreadMalloc.h"
#include "PthreadScheduler.h"
#include "PthreadJoin.h"
#include "PthreadCancel.h"
#include "PthreadCleanup.h"
#include "PthreadCond.h"
#include "PthreadMutex.h"
#include "PthreadKey.h"

namespace PinPthread 
{

    class PthreadSim 
    {
      public:
        PthreadSim();
        ~PthreadSim();
      public:
        int   pthread_cancel(pthread_t);
        void  pthread_cleanup_pop_(int, CONTEXT*);
        void  pthread_cleanup_push_(ADDRINT, ADDRINT);
        int   pthread_cond_broadcast(pthread_cond_t*);
        int   pthread_cond_destroy(pthread_cond_t*);
        int   pthread_cond_signal(pthread_cond_t*);
        void  pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, const struct timespec*, CHECKPOINT*);
        void  pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*, CHECKPOINT*);
        void  pthread_create(pthread_t*, pthread_attr_t*, CONTEXT*, ADDRINT, ADDRINT, CHECKPOINT*);
        int   pthread_detach(pthread_t);
        int   pthread_equal(pthread_t, pthread_t);
        void  pthread_exit(void*, CONTEXT*);
        int   pthread_getattr(pthread_t, pthread_attr_t*);
        void* pthread_getspecific(pthread_key_t);
        void  pthread_join(pthread_t, void**, CHECKPOINT*, CONTEXT*);
        int   pthread_key_create(pthread_key_t*, void(*)(void*));
        int   pthread_key_delete(pthread_key_t);
        int   pthread_kill(pthread_t, int);
        void  pthread_mutex_lock(pthread_mutex_t*, CHECKPOINT*);
        int   pthread_mutex_trylock(pthread_mutex_t*);
        int   pthread_mutex_unlock(pthread_mutex_t*);
        pthread_t pthread_self();
        int   pthread_setcancelstate(int, int*);
        int   pthread_setcanceltype(int, int*);
        int   pthread_setspecific(pthread_key_t, void*);
        void  pthread_testcancel(CONTEXT*);
      public:
        bool inmtmode();
        void  docontextswitch(CHECKPOINT*);
      public: 
        void  threadsafemalloc(bool, bool, const string*);
      private:
        pthread_t new_thread_id;               // the id to assign to the next new thread
        PthreadMalloc* mallocmanager;          // ensure thread-safe memory allocation
        PthreadScheduler* scheduler;           // schedule the threads
        PthreadJoinManager* joinmanager;       // join and detaches the threads
        PthreadCancelManager* cancelmanager;   // track cancellation state for all threads
        PthreadCleanupManager* cleanupmanager; // responsible for cleaning up the threads
        PthreadCondManager* condmanager;       // blocks and unblocks threads on condition variables
        PthreadMutexManager* mutexmanager;     // block and unblock threads on mutexes
        PthreadTLSManager* tlsmanager;         // manage thread local storage key/data
    };
    
}  // namespace PinPthread
    
#endif  // #ifndef PTHREAD_SIM_H
    
