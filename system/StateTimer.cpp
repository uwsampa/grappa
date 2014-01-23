////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////
#include "StateTimer.hpp"
#include "Grappa.hpp"

StateTimer * state_timer;

/// Put the state of the timer into state of the current thread
void StateTimer::enterState_thread_( ) {
#if STATE_TIMER_ON
    switch ( Grappa::current_worker()->state ) {
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

/// Set the StateTimer state of the Worker
void StateTimer::setThreadState_( int state ) {
#if STATE_TIMER_ON
    Grappa::current_worker()->state = state;
#endif
}
