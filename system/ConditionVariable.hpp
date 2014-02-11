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

#ifndef __CONDITION_VARIABLE_HPP__
#define __CONDITION_VARIABLE_HPP__

#include "Message.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "ConditionVariableLocal.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{
  
  /// Proxy for remote ConditionVariable manipulation
  /// @todo: implement
  inline void wait( GlobalAddress<ConditionVariable> m ) {
    // if local, just wait
    // if remote, spawn a task on the home node to wait
  }

  template<typename ConditionVariable>
  inline void signal( const GlobalAddress<ConditionVariable> m ) {
    if (m.core() == Grappa::mycore()) {
      // if local, just signal
      Grappa::signal(m.pointer());
    } else {
      // if remote, signal via active message
      send_heap_message(m.core(), [m]{
        Grappa::signal(m.pointer());
      });
    }
  }

  /// TODO: implement
  inline void signal_all( GlobalAddress<ConditionVariable> m ) {
    // if local, just signal
    // if remote, signal via active message
  }

  /// @}

}

#endif
