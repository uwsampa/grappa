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
#ifndef STATE_TIMER_HPP
#define STATE_TIMER_HPP

#ifdef GRAPPA_TRACE
#include <TAU.h>
#else
// need to define these when tau is disabled
// because for some reason they are not even blank defined,
// unlike other Tau api calls
    #define TAU_CREATE_TASK( a )
    #define TAU_PROFILER_START_TASK( a, b )
    #define TAU_PROFILER_STOP_TASK( a, b )
#endif

#include <glog/logging.h>

#ifdef GRAPPA_TRACE
#define STATE_TIMER_ON 0
#else 
#define STATE_TIMER_ON 0
#endif

class StateTimer;
extern StateTimer * state_timer;

#define ST_START(timer) TAU_PROFILER_START_TASK(timer, tau_taskid)
#define ST_STOP(timer) TAU_PROFILER_STOP_TASK(timer, tau_taskid)

class StateTimer {
    private:

    void * top_level_timer;
    void * user_timer,
         * system_timer,
         * communication_timer,
         * deaggregation_timer,
         * scheduler_timer,
         * findwork_timer;

    void * current_timer;
    int tau_taskid;

    StateTimer() {
#if STATE_TIMER_ON
        // top level timer
        TAU_PROFILER_CREATE( top_level_timer, "state_timing", "(top level)", GRAPPA_STATE_TIMER_GROUP );

        // create the timers
        TAU_PROFILER_CREATE( user_timer, "user_timer", "()", GRAPPA_STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( system_timer, "system_timer", "()", GRAPPA_STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( communication_timer, "communication_timer", "()", GRAPPA_STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( deaggregation_timer, "deaggregation_timer", "()", GRAPPA_STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( scheduler_timer, "scheduler_timer", "()", GRAPPA_STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( findwork_timer, "findwork_timer", "()", GRAPPA_STATE_TIMER_GROUP );

        // Create a fake task to make State timers separate from normal profiling.
        // This is necessary because Tau expects only nested timers
        TAU_CREATE_TASK( tau_taskid );

        ST_START( top_level_timer );

        // assume created in system
        current_timer = system_timer;
        ST_START( system_timer );
#endif
    }

    ~StateTimer() {
#if STATE_TIMER_ON
        ST_STOP( current_timer );
        ST_STOP( top_level_timer );
#endif
    }

    void enterState_user_() {
#if STATE_TIMER_ON
        CHECK( current_timer != user_timer );

        ST_STOP( current_timer );
        current_timer = user_timer;
        ST_START( user_timer );
#endif
    }

    void enterState_system_() {
#if STATE_TIMER_ON
        CHECK( current_timer != system_timer );

        ST_STOP( current_timer );
        current_timer = system_timer;
        ST_START( system_timer );
#endif
    }
    
    void enterState_communication_() {
#if STATE_TIMER_ON
        CHECK( current_timer != communication_timer );

        ST_STOP( current_timer );
        current_timer = communication_timer;
        ST_START( communication_timer );
#endif
    }
    
    void start_communication_() {
#if STATE_TIMER_ON
        CHECK( current_timer != communication_timer );
        ST_START( communication_timer );
#endif
    }
    
    void stop_communication_() {
#if STATE_TIMER_ON
        CHECK( current_timer != communication_timer );
        ST_STOP( communication_timer );
#endif
    }

    void enterState_deaggregation_() {
#if STATE_TIMER_ON
        CHECK( current_timer != deaggregation_timer );

        ST_STOP( current_timer );
        current_timer = deaggregation_timer;
        ST_START( deaggregation_timer );
#endif
    }

    void enterState_scheduler_() {
#if STATE_TIMER_ON
        CHECK( current_timer != scheduler_timer );

        ST_STOP( current_timer );
        current_timer = scheduler_timer;
        ST_START( scheduler_timer );
#endif
    }
    
    void enterState_findwork_() {
#if STATE_TIMER_ON
        CHECK( current_timer != findwork_timer );

        ST_STOP( current_timer );
        current_timer = findwork_timer;
        ST_START( findwork_timer );
#endif
    }

    void enterState_thread_();

    void setThreadState_( int state );
    
    public:
    static const int USER = 1;
    static const int FINDWORK = 2;
    static const int SYSTEM = 3;
    static const int SCHEDULER = 4;
    static const int COMMUNICATION = 5;
    static const int DEAGGREGATION = 6;

    static void init() { state_timer = new StateTimer(); }
    static void finish() { delete state_timer; }

    static void enterState_user() {
#if STATE_TIMER_ON
      state_timer->enterState_user_();
#endif
    }
    static void enterState_system() {
#if STATE_TIMER_ON
      state_timer->enterState_system_();
#endif
    }
    static void enterState_communication() {
#if STATE_TIMER_ON
      state_timer->enterState_communication_();
#endif
    }
    static void enterState_deaggregation() {
#if STATE_TIMER_ON
      state_timer->enterState_deaggregation_();
#endif
    }
    static void enterState_scheduler() {
#if STATE_TIMER_ON
      state_timer->enterState_scheduler_();
#endif
    }
    static void enterState_findwork() {
#if STATE_TIMER_ON
      state_timer->enterState_findwork_();
#endif
    }
    static void enterState_thread() {
#if STATE_TIMER_ON
      state_timer->enterState_thread_();
#endif
    }
    static void setThreadState(int state) {
#if STATE_TIMER_ON
      state_timer->setThreadState_( state );
#endif
    }

    static void start_communication() {
#if STATE_TIMER_ON
      state_timer->start_communication_();
#endif
    }
    static void stop_communication() {
#if STATE_TIMER_ON
      state_timer->stop_communication_();
#endif
    }
};


#endif // STATE_TIMER_HPP
