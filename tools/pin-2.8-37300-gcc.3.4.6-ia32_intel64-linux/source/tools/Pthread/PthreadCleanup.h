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
/* PthreadCleanup:                                                             */
/* manages the cleanup handlers of all threads                                 */
/* --------------------------------------------------------------------------- */

#ifndef PTHREAD_CLEANUP_H
#define PTHREAD_CLEANUP_H

#include "PthreadUtil.h"

namespace PinPthread 
{

    typedef std::pair<ADDRINT, ADDRINT>    pthread_handler_t;
    typedef std::vector<pthread_handler_t> pthread_handlerlifo_t;
    
    class PthreadCleanupHandlers
    {
      public:
        void PushHandler(ADDRINT, ADDRINT);
        pthread_handler_t PopHandler();
        bool HasMoreHandlers();
      public:
        pthread_handlerlifo_t handlerlifo;   // list of <handler, arg>
    };

    typedef std::map<pthread_t, PthreadCleanupHandlers*> pthreadhandler_queue_t;

    class PthreadCleanupManager 
    {
      public:
        void PushHandler(pthread_t, ADDRINT, ADDRINT);
        void PopHandler(pthread_t, int, CONTEXT*);
      private:
        PthreadCleanupHandlers* GetHandlerLIFO(pthread_t);        
      private:
        pthreadhandler_queue_t handlers;     // list of handlers for each thread
    };
    
} // namespace PinPthread

#endif  // #ifndef PTHREAD_CLEANUP_H
