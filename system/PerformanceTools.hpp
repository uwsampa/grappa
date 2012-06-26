#ifndef PERFORMANCE_TOOLS_HPP
#define PERFORMANCE_TOOLS_HPP

#include <TAU.h>
#include "CurrentThread.hpp"
#include <gflags/gflags.h>
#include <glog/logging.h>


// need to define these when tau is disabled
// because for some reason they are not even blank defined,
// unlike other Tau api calls
#ifndef GRAPPA_TRACE
    #define TAU_DB_DUMP_PREFIX_TASK( a, b )
#endif

DECLARE_bool(record_grappa_events);

#define SAMPLE_RATE (1<<4)

#define GRAPPA_DEFINE_EVENT_GROUP(group) \
  DEFINE_bool( record_##group, true, "Enable tracing of events in group.")

#define GRAPPA_DECLARE_EVENT_GROUP(group) \
  DECLARE_bool( record_##group )

#ifdef GRAPPA_TRACE
#define GRAPPA_EVENT(name, text, frequency, group, val) \
  static uint64_t name##_calls = 0; \
  name##_calls++; \
  if (FLAGS_##record_##group && (FLAGS_record_grappa_events) && (name##_calls % frequency == 0)) { \
    TAU_REGISTER_EVENT(name, text); \
    TAU_EVENT(name, val); \
  } \
  do {} while (0)
#else
#define GRAPPA_EVENT(name, text, frequency, group, val) do {} while (0)
#endif


// These are specifically meant to profile the current (or a given) grappa thread
#ifdef GRAPPA_TRACE
#define GRAPPA_PROFILE_CREATE(timer, nametext, typetext, group) \
    void * timer; \
    TAU_PROFILER_CREATE(timer, nametext, typetext, group)
#define GRAPPA_PROFILE_THREAD_START(timer, thread) TAU_PROFILER_START_TASK( timer, Grappa_tau_id((thread)) )
#define GRAPPA_PROFILE_THREAD_STOP(timer, thread) TAU_PROFILER_STOP_TASK( timer, Grappa_tau_id((thread)) )
#define GRAPPA_PROFILE_START(timer) GRAPPA_PROFILE_THREAD_START( timer, Grappa_current_thread() ) 
#define GRAPPA_PROFILE_STOP(timer) GRAPPA_PROFILE_THREAD_STOP( timer, Grappa_current_thread() ) 
#else
#define GRAPPA_PROFILE_CREATE(timer, nametext, typetext, group) do {} while (0)
#define GRAPPA_PROFILE_THREAD_START(timer, thread) do {} while (0)
#define GRAPPA_PROFILE_THREAD_STOP(timer, thread) do {} while (0)
#define GRAPPA_PROFILE_START(timer) do {} while (0)
#define GRAPPA_PROFILE_STOP(timer) do {} while (0)  
#endif


// Convenient C++ scope profiling macros
#ifdef GRAPPA_TRACE
#define __cat2(x,y) x##y
#define __cat(x,y) __cat2(x,y)
#define GRAPPA_THREAD_PROFILE( timer, nametext, typetext, group, thread ) \
    GRAPPA_PROFILE_CREATE( timer, nametext, typetext, group ); \
    GrappaProfiler __cat( prof, __LINE__) ( timer, thread )

#define GRAPPA_PROFILE( timer, nametext, typetext, group ) GRAPPA_THREAD_PROFILE( timer, nametext, typetext, group, Grappa_current_thread() )
#include <boost/current_function.hpp>
#define GRAPPA_FUNCTION_PROFILE( group ) GRAPPA_PROFILE( __cat(timer, __LINE__), BOOST_CURRENT_FUNCTION, "", group )
#define GRAPPA_THREAD_FUNCTION_PROFILE( group, thread ) GRAPPA_THREAD_PROFILE( __cat(timer, __LINE__), BOOST_CURRENT_FUNCTION, "", group, thread )

class Thread;
class GrappaProfiler {
    private:
        void* timer;
        Thread * thr;

    public:
        GrappaProfiler ( void* timer, Thread * thr ) 
            : timer ( timer )
            , thr ( thr ) {
            
                GRAPPA_PROFILE_THREAD_START( timer, thr );
        }

        ~GrappaProfiler ( ) {
            GRAPPA_PROFILE_THREAD_STOP( timer, thr );
        }
};
#else
#define GRAPPA_THREAD_PROFILE( timer, nametext, typetext, group, thread ) do {} while(0)
#define GRAPPA_PROFILE( timer, nametext, typetext, group ) do {} while(0)
#define GRAPPA_FUNCTION_PROFILE( group ) do {} while(0)
#define GRAPPA_THREAD_FUNCTION_PROFILE( group, thread ) do {} while(0)
#endif



void dump_all_task_profiles();


// include profiler groups
#include "ProfilerGroups.hpp"


void SoftXMT_set_profiler_argv0( char * argv0 );

void SoftXMT_start_profiling();
void SoftXMT_stop_profiling();

extern bool take_profiling_sample;

#endif // PERFORMANCE_TOOLS_HPP

