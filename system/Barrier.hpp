#pragma once

#include "ConditionVariable.hpp"
#include "Communicator.hpp"

namespace Grappa {
  extern ConditionVariable barrier_cv;
  
  /// Mostly equivalent to Grappa_barrier_suspending but uses ConditionVariable.
  inline void barrier() {
    DVLOG(5) << "entering barrier";
    global_communicator.barrier_notify();
    wait(&barrier_cv);
  }
  
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
