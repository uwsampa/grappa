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
#pragma once

#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"
#include "ConditionVariableLocal.hpp"
#include "CountingSemaphoreLocal.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{

  template< typename Semaphore >
  inline void increment( Semaphore * s, int64_t incr = 1 ) {
    s->increment( incr );
  }
  
  template< typename Semaphore >
  inline void decrement( Semaphore * s, int64_t decr = 1 ) {
    s->decrement( decr );
  }
  
  template< typename Semaphore >
  inline bool try_decrement( Semaphore * s, int64_t decr = 1 ) {
    return s->try_decrement( decr );
  }
  
  template< typename Semaphore >
  inline int64_t get_value( Semaphore * s ) {
    return s->get_value();
  }
  
  /// @}

}

