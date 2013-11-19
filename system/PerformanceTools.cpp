
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
    
/// set when we should take a sample at the next context switch
bool take_profiling_sample = false;

// remember argv[0] for generating profiler filenames
static char * argv0_for_profiler = NULL;
int time_for_profiler = 0;

#define PROFILER_FILENAME_LENGTH 2048
static char profiler_filename[PROFILER_FILENAME_LENGTH] = {0}; 

// which profiling phase are we in? 
static int profiler_phase = 0;

/// Record binary name for use in construcing profiler filenames
void Grappa_set_profiler_argv0( char * argv0 ) {
  argv0_for_profiler = argv0;
  time_for_profiler = time(NULL);
}

/// Get next filename for profiler files
char * Grappa_get_next_profiler_filename( ) {
  // use Slurm environment variables if we can
  // char * jobname = getenv("SLURM_JOB_NAME");
  char * jobid = getenv("SLURM_JOB_ID");
  char * procid = getenv("SLURM_PROCID");
  if( /*jobname != NULL &&*/ jobid != NULL && procid != NULL ) {
    sprintf( profiler_filename, "%s.%s.rank%s.phase%d.prof", "exe", jobid, procid, profiler_phase );
  } else {
    sprintf( profiler_filename, "%s.%d.pid%d.phase%d.prof", argv0_for_profiler, time_for_profiler, getpid(), profiler_phase );
  }
  VLOG(3) << "profiler_filename = " << profiler_filename;
  ++profiler_phase;
  return profiler_filename;
}

/// callback for sampling at profiler signals
/// runs in signal handler, so should be async-signal-safe
int Grappa_profile_handler( void * arg ) {
  take_profiling_sample = true;
#ifdef GOOGLE_PROFILER
  return 1; // tell profiler to profile at this tick
#endif
#ifdef STATS_BLOB
  return 0; // don't take profiling sample
#endif
  return 0;
}

void Grappa_reset_stats();

#include "Statistics.hpp"

void Grappa_start_profiling() {
#ifdef GOOGLE_PROFILER
  ProfilerOptions po;
  po.filter_in_thread = &Grappa_profile_handler;
  po.filter_in_thread_arg = NULL;
  ProfilerStartWithOptions( Grappa_get_next_profiler_filename(), &po );
  //ProfilerStart( Grappa_get_profiler_filename() );
#if defined(VTRACE_SAMPLED) || defined(HISTOGRAM_SAMPLED)
  Grappa_reset_stats();
#endif
#ifdef VTRACE_SAMPLED
  VT_USER_START("sampling");
  Grappa::Statistics::sample();
#endif
#endif // GOOGLE_PROFILER
}

void Grappa_stop_profiling() {
#ifdef GOOGLE_PROFILER
    ProfilerStop( );
    Grappa_profile_handler(NULL);
#ifdef VTRACE_SAMPLED
  VT_USER_END("sampling");
  // void Grappa_dump_stats( std::ostream& oo = std::cout );
  // Grappa_dump_stats();

  Grappa_reset_stats();
  Grappa::Statistics::sample();
#endif
#endif
}

