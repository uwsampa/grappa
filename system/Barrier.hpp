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
