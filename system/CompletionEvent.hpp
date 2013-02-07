#pragma once

#include "Grappa.hpp"
#include "ConditionVariableLocal.hpp"
#include "Message.hpp"
#include "Tasking.hpp"

namespace Grappa {

  /// Synchronization primitive useful for waking a worker after a number of other things complete.
  /// All waiting tasks will be woken as soon as the count goes to 0.
  /// Use `enroll()` to add more tasks.
  ///
  /// Fulfills the ConditionVariable type_trait.
  class CompletionEvent {
    ConditionVariable cv;
    int64_t count;
  public:
    CompletionEvent(int64_t count = 0): count(count) {}
    
    void enroll(int64_t inc = 1) {
      count += inc;
    }
    
    /// Decrement count once, if count == 0, wake all waiters.
    void complete(int64_t decr = 1) {
      if (count-decr < 0) {
        LOG(ERROR) << "too many calls to signal()";
      }
      count -= decr;
      if (count == 0) {
        broadcast(&cv);
      }
    }
    
    void wait() {
      if (count > 0) {
        Grappa::wait(&cv);
      }
    }
  };

  /// Match ConditionVariable-style function call.
  inline void complete( CompletionEvent * ce ) {
    ce->complete();
  }
  
  /// Overload to work on GlobalAddresses.
  inline void complete(GlobalAddress<CompletionEvent> ce, int64_t decr = 1) {
    if (ce.node() == mycore()) {
      ce.pointer()->complete(decr);
    } else {
      if (decr == 1) {
        // (common case) don't send full 8 bytes just to decrement by 1
        send_message(ce.node(), [ce] {
          ce.pointer()->complete();
        });
      } else {
        send_message(ce.node(), [ce,decr] {
          ce.pointer()->complete(decr);
        });
      }
    }
  }
  
  template<typename TF>
  void privateTask(CompletionEvent * ce, TF tf) {
    ce->enroll();
    privateTask([ce,tf] {
      tf();
      ce->complete();
    });
  }
  
} // namespace Grappa
