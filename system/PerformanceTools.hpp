////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

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


