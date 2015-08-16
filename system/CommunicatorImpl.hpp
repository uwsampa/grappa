////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "Communicator.hpp"
#include "CompletionEvent.hpp"

template< typename F >
void Communicator::with_request_do_blocking( F f ) {
  CommunicatorContext c;
  if( global_communicator.collective_context ) {
    LOG(ERROR) << "Only one outstanding collective operation allowed.";
    CHECK_NULL( global_communicator.collective_context );
  }

  // record that context has been issued
  global_communicator.collective_context = &c;
  

  Grappa::CompletionEvent ce(1); // register ourselves
  
  // wake calling thread when done
  c.buf = (void*) &ce;   // this is a hack since the callback type is not templated
  c.callback = [] ( CommunicatorContext * c, int source, int tag, int received_size ) {
    c->reference_count = 0;
    auto ce = (Grappa::CompletionEvent*) c->buf;
    ce->complete();
  };
  c.reference_count = 1; // will be set to 0 after request is done
  
  // let caller do stuff with context's request
  f(&c.request);
  
  // suspend thread until context is done
  ce.wait();
}
