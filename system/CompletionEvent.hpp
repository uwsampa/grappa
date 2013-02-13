#pragma once

#include <type_traits>
#include "Communicator.hpp"
#include "ConditionVariableLocal.hpp"
#include "Message.hpp"
#include "Tasking.hpp"
#include "MessagePool.hpp"
#include "Delegate.hpp"

namespace Grappa {

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
  template<typename CompletionType>
  inline void complete( CompletionType * ce ) {
    static_assert(std::is_base_of<CompletionEvent,CompletionType>::value,
                  "complete() can only be called on subclasses of CompletionEvent");
    ce->complete();
  }
  
  /// Overload to work on GlobalAddresses.
  template<typename CompletionType>
  inline void complete(GlobalAddress<CompletionType> ce, int64_t decr = 1) {
    static_assert(std::is_base_of<CompletionEvent,CompletionType>::value,
                  "complete() can only be called on subclasses of CompletionEvent");
    
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
  
  
  class GlobalCompletionEvent : public CompletionEvent {
    // All nodes
    bool global_done;
    bool cancel_in_flight;
    bool enter_called;
    
    Core master_core;
    
    // Master barrier only
    Core cores_in;
    bool barrier_done; // only for verification
    
    /// Notify the master joiner that this joiner doesn't have any work to do for now.
    void send_enter() {
      enter_called = true;
      if (!cancel_in_flight) {
        auto* ce = this; // assume an identical GlobalCompletionEvent on each node (static globals)
        send_message(master_core, [ce]() {
          CHECK(mycore() == 0); // Core 0 is Master
          
          ce->cores_in++;
          VLOG(2) << "(completed) cores_in: " << ce->cores_in;
          
          if (ce->cores_in == cores()) {
            ce->barrier_done = true;
            VLOG(2) << "### done! ### cores_in: " << ce->cores_in;
            
            MessagePool<(1<<16)> pool;
            for (Core c=0; c<cores(); c++) {
              pool.send_message(c, [ce]() {
                ce->wake();
              });
            }
            //blocks until all wake messages sent
          }
        });
      }
    }
    
    void send_cancel() {
      if (!cancel_in_flight && enter_called) {
        cancel_in_flight = true;
        enter_called = false;

        auto master_ce = make_global(this, master_core);
        auto master_cores_in = global_pointer_to_member(master_ce, &GlobalCompletionEvent::cores_in);
        int64_t result = delegate::fetch_and_add(master_cores_in, -1);
        VLOG(2) << "(cancelled) cores_in: " << result-1;

        cancel_in_flight = false;
       
        CHECK(count > 0);
      }
    }
    
    /// Wake this node's suspended task and set joiner to the completed state to ensure that
    /// any subsequent calls to wait() will fall through (until reset() is called of course).
    void wake() {
      VLOG(2) << "wake called";
      CHECK(!global_done);
      CHECK(count == 0) << "count == " << count;
      global_done = true;
      broadcast(&cv);
    }
    
  public:
    void reset() {
      CHECK(cv.waiters_ == 0) << "resetting when tasks are still waiting";
      cores_in = 0;
      global_done = false;
      cancel_in_flight = false;
      enter_called = false;
    }
    
    void enroll(int64_t inc = 1) {
      if (count == 0) { // 0 -> >=1
        send_cancel();
      }
      count += inc;
    }
    
    void complete(int64_t dec = 1) {
      CHECK(count >= dec) << "too many calls to complete(), count == " << count;
      
      count -= dec;
      VLOG(4) << "complete => " << count;
      
      if (count == 0) { // >0 -> 0
//        if (cv.waiters_ == 0) {
          send_enter();
//        }
      }
    }
    
    void wait() {
      if (!global_done) {
        if (!cancel_in_flight && count == 0 && !enter_called) {
          // TODO: doing extra 'cancel_in_flight' check
          send_enter();
        }
        Grappa::wait(&cv);
        
        // verify things on finally waking
        CHECK(global_done);
        CHECK(!cancel_in_flight);
        CHECK(count == 0);
      }
    }
  };
  
} // namespace Grappa
