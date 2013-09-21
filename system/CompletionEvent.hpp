#pragma once

#include <type_traits>
#include "Communicator.hpp"
#include "ConditionVariableLocal.hpp"
#include "Message.hpp"
#include "Tasking.hpp"
#include "MessagePool.hpp"
#include "Delegate.hpp"

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, ce_remote_completions);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, ce_completions);

namespace Grappa {
  /// @addtogroup Synchronization
  /// @{

  /// Synchronization primitive useful for waking a worker after a number of other things complete.
  /// All waiting tasks will be woken as soon as the count goes to 0.
  /// Use `enroll()` to add more tasks.
  ///
  /// Fulfills the ConditionVariable type_trait.
  class CompletionEvent {
  protected:
    ConditionVariable cv;
    int64_t count;
  public:
    CompletionEvent(int64_t count = 0): count(count) {}

    int64_t get_count() const { return count; }
    
    void enroll(int64_t inc = 1) {
      count += inc;
    }
    
    /// Decrement count once, if count == 0, wake all waiters.
    void complete(int64_t decr = 1) {
      ce_completions += decr;
      CHECK_GE( count-decr, 0 ) << "too many calls to signal()";
      count -= decr;
      DVLOG(4) << "completed (" << count << ")";
      if (count == 0) {
        broadcast(&cv);
      }
    }
    
    void wait() {
      if (count > 0) {
        Grappa::wait(&cv);
      }
    }
    
    void wait(SuspendedDelegate * c) {
      CHECK( is_suspended_delegate((Worker*)c) );
      if (count > 0) {
        Grappa::add_waiter(&cv, c);
      } else {
        invoke(c);
      }
    }

    void reset() {
      CHECK_EQ( cv.waiters_, 0 ) << "Resetting with waiters!";
      count = 0;
    }
  };
  
  /// Match ConditionVariable-style function call.
  template<typename CompletionType>
  inline void complete( CompletionType * ce ) {
    static_assert(std::is_base_of<CompletionEvent,CompletionType>::value,
                  "complete() can only be called on subclasses of CompletionEvent");
    ce->complete();
  }
  
  /// Overload to work on GlobalAddresses.
  // template<typename CompletionType>
  // inline void complete(GlobalAddress<CompletionType> ce, int64_t decr = 1) {
    // static_assert(std::is_base_of<CompletionEvent,CompletionType>::value,
    //               "complete() can only be called on subclasses of CompletionEvent");
  inline void complete(GlobalAddress<CompletionEvent> ce, int64_t decr = 1) {
    DVLOG(5) << "complete CompletionEvent";
    if (ce.node() == mycore()) {
      ce.pointer()->complete(decr);
    } else {
      ce_remote_completions += decr;
      if (decr == 1) {
        // (common case) don't send full 8 bytes just to decrement by 1
        send_heap_message(ce.node(), [ce] {
          ce.pointer()->complete();
        });
      } else {
        send_heap_message(ce.node(), [ce,decr] {
          ce.pointer()->complete(decr);
        });
      }
    }
  }
  
  inline void enroll(GlobalAddress<CompletionEvent> ce, int64_t incr = 1) {
    delegate::call(ce, [incr](CompletionEvent * c){ c->enroll(incr); });
  }
  
  /// Spawn Grappa::privateTask and implicitly synchronize with the given CompletionEvent 
  /// (or GlobalCompletionEvent, though if using GlobalCompletionEvent, it may be better 
  /// to use the verison that takes the GCE pointer as a template parameter only).
  template<typename CompletionType, typename TF>
  void privateTask(CompletionType * ce, TF tf) {
    static_assert(std::is_base_of<CompletionEvent,CompletionType>::value,
                  "Synchronizing privateTask spawn must take subclass of CompletionEvent as argument");
    ce->enroll();
    privateTask([ce,tf] {
      tf();
      ce->complete();
    });
  }
  
  /// @}
  
} // namespace Grappa
