#include "StateTimer.hpp"
#include "SoftXMT.hpp"

StateTimer * state_timer;

void StateTimer::enterState_thread_( ) {
#ifdef GRAPPA_TRACE
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

void StateTimer::setThreadState_( int state ) {
#ifdef GRAPPA_TRACE
    CURRENT_THREAD->state = state;
#endif
}
