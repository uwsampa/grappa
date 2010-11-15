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
/* PthreadKey:                                                                 */
/* manages thread local storage                                                */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_KEY_H
#define PTHREAD_KEY_H

#include "PthreadUtil.h"

namespace PinPthread 
{

    typedef std::map<pthread_key_t, void*>          pthread_tls_t;
    
    class PthreadTLS
    {
      public:
        void AddKey(pthread_key_t);
        bool RemoveKey(pthread_key_t);
        int SetData(pthread_key_t, void*);
        void* GetData(pthread_key_t);
      public:        
        pthread_tls_t tls;           // <key,data> pairs for one thread
    };

    typedef std::map<pthread_t, PthreadTLS*> pthreadtls_queue_t;
    typedef std::map<pthread_key_t, void(*)(void*)> pthread_keydestr_t;

    class PthreadTLSManager 
    {
       public:
        PthreadTLSManager();
        ~PthreadTLSManager();
       public:
        void AddThread(pthread_t);
        void KillThread(pthread_t);
        int AddKey(pthread_key_t*, void(*)(void*));
        int RemoveKey(pthread_key_t);
        int SetData(pthread_t, pthread_key_t, void*);
        void* GetData(pthread_t, pthread_key_t);
       private:
        PthreadTLS* GetTLS(pthread_t);
       private:
        pthread_key_t newkey;        // the value to assign to the next key
        pthreadtls_queue_t tls;      // thread local storage for each thread
        pthread_keydestr_t destr;    // destructor function for each key
    };
    
} // namespace PinPthread

#endif  // #ifndef PTHREAD_KEY_H
