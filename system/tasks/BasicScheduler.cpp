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
#include "BasicScheduler.hpp"

/// TODO: this should be based on some actual time-related metric so behavior is predictable across machines
// DEFINE_int64( periodic_poll_ticks, 500, "number of ticks to wait before polling periodic queue");

namespace Grappa {

void BasicScheduler::run ( ) {
    while (thread_wait( NULL ) != NULL) { } // nothing
}

Worker * BasicScheduler::thread_wait( void **result ) {
    CHECK( current_thread == master ) << "only meant to be called by system Worker";

    Worker * next = nextCoroutine( false );
    if (next == NULL) {
        // no user threads remain
        return NULL;
    } else {
        current_thread = next;

        Worker * died = (Worker *) impl::thread_context_switch( master, next, NULL);

        // At the moment, we only return from a Worker in the case of death.

        if (result != NULL) {
            void *retval = (void *)died->next;
            *result = retval;
        }
        return died;
    }
}

} // namespace Grappa
