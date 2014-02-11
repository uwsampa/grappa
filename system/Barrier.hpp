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

#include "ConditionVariable.hpp"
#include "Communicator.hpp"

namespace Grappa {
  /// @addtogroup Synchronization
  /// @{
  
  extern ConditionVariable barrier_cv;
  
  /// Blocking SPMD barrier (must be called once on all cores to continue)
  inline void barrier() {
    DVLOG(5) << "entering barrier";
    global_communicator.barrier_notify();
    wait(&barrier_cv);
  }
  
  /// Directly call the underlying communicator's blocking barrier. This is not
  /// recommended, as it prevents Grappa communication from continuing.
  inline void comm_barrier() {
    global_communicator.barrier();
  }
  
namespace impl {
  /// Called by polling thread (Grappa.cpp::poller). On barrier completion, calls `broadcast` on all cores' `barrier_cv`s.
  inline void barrier_poll() {
    // skip if no one is waiting on the barrier
    if (barrier_cv.waiters_ != 0) {
      DVLOG(5) << "trying barrier";
      if (global_communicator.barrier_try()) {
        broadcast(&barrier_cv);
      }
    }
  }
}
  
  /// @}
}
