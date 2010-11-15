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
/* PthreadJoin:                                                                */
/* manages the interthread interaction of joining and detaching                */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_JOIN_H
#define PTHREAD_JOIN_H

#include "PthreadUtil.h"

namespace PinPthread 
{

    class PthreadJoinee
    {
      public:
        PthreadJoinee();
        ~PthreadJoinee();
      public:
        bool done;          // whether this thread has finished execution
        bool joined;        // whether there is a thread waiting to join this thread
        pthread_t joiner;   // thread waiting to join this thread
        void* retval;       // this thread's return value, upon completion
    };
    
    class PthreadJoiner
    {
      public:
        PthreadJoiner(pthread_t, void**);
        ~PthreadJoiner();
      public:
        pthread_t joinee;   // which thread this thread is trying to join
        void** retvalptr;   // where to store the joinee's return value
    };

    typedef std::map<pthread_t, PthreadJoiner*> pthreadjoiner_queue_t;
    typedef std::map<pthread_t, PthreadJoinee*> pthreadjoinee_queue_t;
    
    class PthreadJoinManager
    {
      public:
        void AddThread(pthread_t, pthread_attr_t*);
        bool KillThread(pthread_t, void*, pthread_t*);
        int  DetachThread(pthread_t);
        bool JoinThreads(pthread_t, pthread_t, void**);
        void GetAttr(pthread_t, pthread_attr_t*);
      private:
        inline pthreadjoinee_queue_t::iterator GetJoineeptr(pthread_t);
        inline void InsertJoinee(pthread_t);
        inline void EraseJoinee(pthread_t);
        inline bool IsJoinable(pthread_t);
        inline bool IsDone(pthread_t);
        inline bool IsJoined(pthread_t);
        inline pthread_t GetJoiner(pthread_t);
        inline void SetJoiner(pthread_t, pthread_t);
        inline void* GetRetval(pthread_t);
        inline void SetRetval(pthread_t, void*);
        inline pthreadjoiner_queue_t::iterator GetJoinerptr(pthread_t);
        inline void InsertJoiner(pthread_t, pthread_t, void**);
        inline void EraseJoiner(pthread_t);
        inline pthread_t GetJoinee(pthread_t);
        inline void** GetRetvalptr(pthread_t);
      private:
        pthreadjoinee_queue_t joinables; // list of joinable threads
        pthreadjoiner_queue_t joinings;  // list of threads trying to join
    };
    
} // namespace PinPthread

#endif  // #ifndef PTHREAD_JOIN_H
