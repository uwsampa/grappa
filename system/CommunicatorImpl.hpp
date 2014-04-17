////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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
