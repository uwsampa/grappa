// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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

    static void enterState_user() { state_timer->enterState_user_(); }
    static void enterState_system() { state_timer->enterState_system_(); }
    static void enterState_communication() { state_timer->enterState_communication_(); }
    static void enterState_deaggregation() { state_timer->enterState_deaggregation_(); }
    static void enterState_scheduler() { state_timer->enterState_scheduler_(); }
    static void enterState_findwork() { state_timer->enterState_findwork_(); }
    static void enterState_thread() { state_timer->enterState_thread_(); }
    static void setThreadState(int state) { state_timer->setThreadState_( state ); }

    static void start_communication() { state_timer->start_communication_(); }
    static void stop_communication() { state_timer->stop_communication_(); }
};


#endif // STATE_TIMER_HPP
