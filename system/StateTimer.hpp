#ifndef STATE_TIMER_HPP
#define STATE_TIMER_HPP

#include <TAU.h>
#include <glog/logging.h>

#define STATE_TIMER_GROUP TAU_USER3

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
        // top level timer
        TAU_PROFILER_CREATE( top_level_timer, "state_timing", "(top level)", STATE_TIMER_GROUP );

        // create the timers
        TAU_PROFILER_CREATE( user_timer, "user_timer", "()", STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( system_timer, "system_timer", "()", STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( communication_timer, "communication_timer", "()", STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( deaggregation_timer, "deaggregation_timer", "()", STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( scheduler_timer, "scheduler_timer", "()", STATE_TIMER_GROUP );
        TAU_PROFILER_CREATE( findwork_timer, "findwork_timer", "()", STATE_TIMER_GROUP );

        // Create a fake task to make State timers separate from normal profiling.
        // This is necessary because Tau expects only nested timers
        TAU_CREATE_TASK( tau_taskid );

        ST_START( top_level_timer );

        // assume created in system
        current_timer = system_timer;
        ST_START( system_timer );
    }

    ~StateTimer() {
        ST_STOP( current_timer );
        ST_STOP( top_level_timer );
    }

    void enterState_user_() {
        CHECK( current_timer != user_timer );

        ST_STOP( current_timer );
        current_timer = user_timer;
        ST_START( user_timer );
    }

    void enterState_system_() {
        CHECK( current_timer != system_timer );

        ST_STOP( current_timer );
        current_timer = system_timer;
        ST_START( system_timer );
    }
    
    void enterState_communication_() {
        CHECK( current_timer != communication_timer );

        ST_STOP( current_timer );
        current_timer = communication_timer;
        ST_START( communication_timer );
    }
    
    void start_communication_() {
        CHECK( current_timer != communication_timer );
        ST_START( communication_timer );
    }
    
    void stop_communication_() {
        CHECK( current_timer != communication_timer );
        ST_STOP( communication_timer );
    }

    void enterState_deaggregation_() {
        CHECK( current_timer != deaggregation_timer );

        ST_STOP( current_timer );
        current_timer = deaggregation_timer;
        ST_START( deaggregation_timer );
    }

    void enterState_scheduler_() {
        CHECK( current_timer != scheduler_timer );

        ST_STOP( current_timer );
        current_timer = scheduler_timer;
        ST_START( scheduler_timer );
    }
    
    void enterState_findwork_() {
        CHECK( current_timer != findwork_timer );

        ST_STOP( current_timer );
        current_timer = findwork_timer;
        ST_START( findwork_timer );
    }

    void enterState_thread_();
    
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

    static void start_communication() { state_timer->start_communication_(); }
    static void stop_communication() { state_timer->stop_communication_(); }
};


#endif // STATE_TIMER_HPP
