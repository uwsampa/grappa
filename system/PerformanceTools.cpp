
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


#include "Thread.hpp"
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
int Grappa_profile_handler( void * arg ) {
  take_profiling_sample = true;
#ifdef GOOGLE_PROFILER
  return 1; // tell profiler to profile at this tick
#endif
#ifdef STATS_BLOB
  return 0; // don't take profiling sample
#endif
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
#ifdef VTRACE_SAMPLED
  VT_USER_START("sampling");
  Grappa_reset_stats();
  Grappa::Statistics::sample();
#endif
#endif
}

void Grappa_stop_profiling() {
#ifdef GOOGLE_PROFILER
    ProfilerStop( );
    Grappa_profile_handler(NULL);
#ifdef VTRACE_SAMPLED
  VT_USER_END("sampling");
  void Grappa_dump_stats( std::ostream& oo = std::cout );
  Grappa_dump_stats();

  Grappa_reset_stats();
  Grappa::Statistics::sample();
#endif
#endif
}

/// User-registered sampled counters
#define MAX_APP_COUNTERS 32
unsigned app_counters[MAX_APP_COUNTERS] = { 0 };
std::string app_counter_names[MAX_APP_COUNTERS];
unsigned app_grp_vt = -1;
uint64_t * app_counter_addrs[MAX_APP_COUNTERS] = { NULL };
uint64_t app_counter_reset_vals[MAX_APP_COUNTERS] = { 0 };
bool app_counter_doReset[MAX_APP_COUNTERS] = { false };
bool app_counter_isInteger[MAX_APP_COUNTERS] = { false };
bool app_counter_isDouble[MAX_APP_COUNTERS] = { false };
int ae_next_id = 0;

/// Take sample of user trace counters
void Grappa_profiling_sample_user() {
#ifdef VTRACE_SAMPLED
  for (int i=0; i<ae_next_id; i++) {
    if( app_counter_isInteger[i] ) {
      VT_COUNT_SIGNED_VAL( app_counters[i], *((int64_t*)app_counter_addrs[i]) );
    } else if( app_counter_isDouble[i] ) {
      VT_COUNT_DOUBLE_VAL( app_counters[i], *((double*)app_counter_addrs[i]) );
    } else {
      VT_COUNT_UNSIGNED_VAL( app_counters[i], *(app_counter_addrs[i]) );
    }
  }
#endif
}

void Grappa_dump_user_stats( std::ostream& o = std::cout, const char * terminator = "" ) {
  o << "   \"UserStats\": { ";
  for (int i=0; i<ae_next_id; i++) {
    if( app_counter_isInteger[i] ) {
      o << "\"" << app_counter_names[i] << "\": " << *((int64_t*)app_counter_addrs[i]);
    } else if( app_counter_isDouble[i] ) {
      o << "\"" << app_counter_names[i] << "\": " << *((double*)app_counter_addrs[i]);
    } else {
      o << "\"" << app_counter_names[i] << "\": " << *(app_counter_addrs[i]);
    }
    if( i != ae_next_id-1 ) o << ", ";
  }
  o << "}" << terminator << std::endl;
}

/// Add a new user trace counter
void Grappa_add_profiling_counter(uint64_t * counter, std::string name, std::string abbrev, bool reset, uint64_t resetVal  ) {
  CHECK( ae_next_id < MAX_APP_COUNTERS );
#ifdef VTRACE_SAMPLED
  if ( app_grp_vt == -1 ) app_grp_vt = VT_COUNT_GROUP_DEF( "App" );
  app_counters[ae_next_id] = VT_COUNT_DEF( name.c_str(), abbrev.c_str(), VT_COUNT_TYPE_UNSIGNED, app_grp_vt );
#endif

  app_counter_names[ae_next_id] = name;
  app_counter_addrs[ae_next_id] = counter;
  app_counter_doReset[ae_next_id] = reset;
  app_counter_isInteger[ae_next_id] = false;
  app_counter_isDouble[ae_next_id] = false;
  app_counter_reset_vals[ae_next_id] = resetVal;

  ++ae_next_id;
}

void Grappa_add_profiling_integer(int64_t * counter, std::string name, std::string abbrev, bool reset, int64_t resetVal  ) {
  CHECK( ae_next_id < MAX_APP_COUNTERS );
#ifdef VTRACE_SAMPLED
  if ( app_grp_vt == -1 ) app_grp_vt = VT_COUNT_GROUP_DEF( "App" );
  app_counters[ae_next_id] = VT_COUNT_DEF( name.c_str(), abbrev.c_str(), VT_COUNT_TYPE_INTEGER, app_grp_vt );
#endif

  app_counter_names[ae_next_id] = name;
  app_counter_addrs[ae_next_id] = (uint64_t*) counter;
  app_counter_doReset[ae_next_id] = reset;
  app_counter_isInteger[ae_next_id] = true;
  app_counter_isDouble[ae_next_id] = false;
  app_counter_reset_vals[ae_next_id] = resetVal;

  ++ae_next_id;
}

void Grappa_add_profiling_value(double * counter, std::string name, std::string abbrev, bool reset, double resetVal  ) {
  CHECK( ae_next_id < MAX_APP_COUNTERS );
#ifdef VTRACE_SAMPLED
  if ( app_grp_vt == -1 ) app_grp_vt = VT_COUNT_GROUP_DEF( "App" );
  app_counters[ae_next_id] = VT_COUNT_DEF( name.c_str(), abbrev.c_str(), VT_COUNT_TYPE_DOUBLE, app_grp_vt );
#endif

  app_counter_names[ae_next_id] = name;
  app_counter_addrs[ae_next_id] = (uint64_t*) counter;
  app_counter_doReset[ae_next_id] = reset;
  app_counter_isInteger[ae_next_id] = false;
  app_counter_isDouble[ae_next_id] = true;
  app_counter_reset_vals[ae_next_id] = resetVal;

  ++ae_next_id;
}

/// Reset user trace counters
void Grappa_reset_user_stats() {
#ifdef VTRACE_SAMPLED
#endif
  for (int i=0; i<ae_next_id; i++) {
    if ( app_counter_doReset[i] ) {
      *(app_counter_addrs[i]) = app_counter_reset_vals[i];
    }
  }
}

