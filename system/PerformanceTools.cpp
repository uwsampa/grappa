
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
