#include "PerformanceTools.hpp"

DEFINE_bool(record_grappa_events, true, "Sampling rate of Grappa tracing events.");


#include "Thread.hpp"
void dump_all_task_profiles() {
#ifdef GRAPPA_TRACE
    TAU_DB_DUMP_PREFIX( "dump" );// dump <node>.0.0
    for ( int t=1; t <= thread_last_tau_taskid; t++ ) {
        TAU_DB_DUMP_PREFIX_TASK( "dump", t ); // dump <node>.0.<1-N>
    }
#endif
}
