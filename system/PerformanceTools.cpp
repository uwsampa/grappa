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


/// set when we should take a sample at the next context switch
bool take_profiling_sample = false;

// remember argv[0] for generating profiler filenames
static char * argv0_for_profiler = NULL;
int time_for_profiler = 0;

#define PROFILER_FILENAME_LENGTH 2048
static char profiler_filename[PROFILER_FILENAME_LENGTH] = {0}; 

// which profiling phase are we in? 
static int profiler_phase = 0;

void SoftXMT_set_profiler_argv0( char * argv0 ) {
  argv0_for_profiler = argv0;
  time_for_profiler = time(NULL);
}

char * SoftXMT_get_next_profiler_filename( ) {
  // use Slurm environment variables if we can
  char * jobname = getenv("SLURM_JOB_NAME");
  char * jobid = getenv("SLURM_JOB_ID");
  char * procid = getenv("SLURM_PROCID");
  if( jobname != NULL && jobid != NULL && procid != NULL ) {
    sprintf( profiler_filename, "%s.%s.rank%s.phase%d.prof", jobname, jobid, procid, profiler_phase );
  } else {
    sprintf( profiler_filename, "%s.%d.pid%d.phase%d.prof", argv0_for_profiler, time_for_profiler, getpid(), profiler_phase );
  }
  ++profiler_phase;
  return profiler_filename;
}

/// callback for sampling at profiler signals
/// runs in signal handler, so should be async-signal-safe
int SoftXMT_profile_handler( void * arg ) {
  take_profiling_sample = true;
  return 1; // tell profiler to profile at this tick
}

void SoftXMT_reset_stats();

void SoftXMT_start_profiling() {
#ifdef GOOGLE_PROFILER
  ProfilerOptions po;
  po.filter_in_thread = &SoftXMT_profile_handler;
  po.filter_in_thread_arg = NULL;
  ProfilerStartWithOptions( SoftXMT_get_next_profiler_filename(), &po );
  //ProfilerStart( SoftXMT_get_profiler_filename() );
#ifdef VTRACE_SAMPLED
  VT_USER_START("sampling");
  SoftXMT_reset_stats();
  void SoftXMT_take_profiling_sample();
#endif
#endif
}

void SoftXMT_stop_profiling() {
#ifdef GOOGLE_PROFILER
    ProfilerStop( );
    SoftXMT_profile_handler(NULL);
#ifdef VTRACE_SAMPLED
  VT_USER_END("sampling");
  SoftXMT_reset_stats();
  void SoftXMT_take_profiling_sample();
#endif
#endif
}
