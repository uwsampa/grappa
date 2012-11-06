// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "StateTimer.hpp"
#include "Grappa.hpp"

StateTimer * state_timer;

/// Put the state of the timer into state of the current thread
void StateTimer::enterState_thread_( ) {
#if STATE_TIMER_ON
    switch ( CURRENT_THREAD->state ) {
        case USER:
            enterState_user_();
            break;
        case FINDWORK:
            enterState_findwork_();
            break;
        case SYSTEM:
            enterState_system_();
            break;
        case SCHEDULER:
            enterState_scheduler_();
            break;
        case COMMUNICATION:
            enterState_communication_();
            break;
        case DEAGGREGATION:
            enterState_deaggregation_();
            break;
        default:
            CHECK( false ) << "not valid thread state";
    }
#endif
}

/// Set the StateTimer state of the Thread
void StateTimer::setThreadState_( int state ) {
#if STATE_TIMER_ON
    CURRENT_THREAD->state = state;
#endif
}
