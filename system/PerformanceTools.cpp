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

#include "PerformanceTools.hpp"

#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif


DEFINE_bool(record_grappa_events, true, "Sampling rate of Grappa tracing events.");


#include "Worker.hpp"
void dump_all_task_profiles() {
#ifdef GRAPPA_TRACE
    TAU_DB_DUMP_PREFIX( "dump" );// dump <node>.0.0
    for ( int t=1; t <= thread_last_tau_taskid; t++ ) {
        TAU_DB_DUMP_PREFIX_TASK( "dump", t ); // dump <node>.0.<1-N>
    }
#endif //GRAPPA_TRACE
}
