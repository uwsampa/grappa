#pragma once

#include "ConditionVariable.hpp"
#include "Communicator.hpp"

extern ConditionVariable barrier_cv;

namespace Grappa {
  
  /// Mostly equivalent to Grappa_barrier_suspending but uses ConditionVariable.
  void barrier() {
    global_communicator.barrier_notify();
    wait(&barrier_cv);
  }
  
  namespace impl {
    struct BarrierBroadcast {
      void operator()() {
        broadcast(&barrier_cv);
      }
    };
  }
  
  /// Called by polling thread (Grappa.cpp::poller). On barrier completion, calls `broadcast` on all cores' `barrier_cv`s.
  inline void barrier_poll() {
    // skip if no one is waiting on the barrier
    if (barrier_cv.waiters_ == 0) {
      if (global_communicator.barrier_try()) {
        Message<impl::BarrierBroadcast> msgs[cores()];
        for (Core c=0; c<cores(); c++) {
          msgs[c].send(c);
        }
      }
    }
  }
}
