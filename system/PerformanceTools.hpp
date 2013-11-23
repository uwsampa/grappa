
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Collection of utilities for monitoring performance
#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>

DECLARE_bool(record_grappa_events);

/// Macros to trace events with Tau

#define SAMPLE_RATE (1<<4)

// These are specifically meant to profile the current (or a given) grappa thread
#ifdef GRAPPA_TRACE
#define GRAPPA_PROFILE_CREATE(timer, nametext, typetext, group) \
    void * timer; \
    TAU_PROFILER_CREATE(timer, nametext, typetext, group)
#define GRAPPA_PROFILE_THREAD_START(timer, thread) TAU_PROFILER_START_TASK( timer, Grappa_tau_id((thread)) )
#define GRAPPA_PROFILE_THREAD_STOP(timer, thread) TAU_PROFILER_STOP_TASK( timer, Grappa_tau_id((thread)) )
#define GRAPPA_PROFILE_START(timer) GRAPPA_PROFILE_THREAD_START( timer, impl::global_scheduler.get_current_thread() ) 
#define GRAPPA_PROFILE_STOP(timer) GRAPPA_PROFILE_THREAD_STOP( timer, impl::global_scheduler.get_current_thread() ) 
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

#define GRAPPA_PROFILE( timer, nametext, typetext, group ) GRAPPA_THREAD_PROFILE( timer, nametext, typetext, group, impl::global_scheduler.get_current_thread() )
#include <boost/current_function.hpp>
#define GRAPPA_FUNCTION_PROFILE( group ) GRAPPA_PROFILE( __cat(timer, __LINE__), BOOST_CURRENT_FUNCTION, "", group )
#define GRAPPA_THREAD_FUNCTION_PROFILE( group, thread ) GRAPPA_THREAD_PROFILE( __cat(timer, __LINE__), BOOST_CURRENT_FUNCTION, "", group, thread )

class Worker;
class GrappaProfiler {
    private:
        void* timer;
        Worker * thr;

    public:
        GrappaProfiler ( void* timer, Worker * thr ) 
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



void Grappa_set_profiler_argv0( char * argv0 );

void Grappa_start_profiling();
void Grappa_stop_profiling();

extern bool take_profiling_sample;

