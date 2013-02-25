#pragma once

#include "Communicator.hpp"
#include "CompletionEvent.hpp"
#include "Tasking.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "Delegate.hpp"
#include "Collective.hpp"

namespace Grappa {

class GlobalCompletionEvent : public CompletionEvent {
  // All nodes
  bool global_done;
  bool cancel_in_flight;
  bool enter_called;
  
  Core master_core;
  
  // Master barrier only
  Core cores_in;
//  bool barrier_done; // only for verification
  
  /// pointer to shared arg for loops that use a GCE
  const void * shared_arg;
  
  /// Notify the master joiner that this joiner doesn't have any work to do for now.
  void send_enter() {
    CHECK(!enter_called) << "double entering";
    enter_called = true;
    if (!cancel_in_flight) { // will get called by `send_cancel` once cancel has completed
      auto* gce = this; // assume an identical GlobalCompletionEvent on each node (static globals)
      send_message(master_core, [gce]() {
        CHECK(mycore() == 0); // Core 0 is Master
        
        gce->cores_in++;
        VLOG(2) << "(completed) cores_in: " << gce->cores_in;
        
        if (gce->cores_in == cores()) {
//          gce->barrier_done = true;
          VLOG(2) << "### done! ### cores_in: " << gce->cores_in;
          
          for (Core c=0; c<cores(); c++) {
            send_heap_message(c, [gce]() {
              gce->wake();
            });
          }
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
      if (enter_called) { send_enter(); }
      else { CHECK(count > 0) << "count is " << count; }
    }
  }
  
  /// Wake this node's suspended task and set joiner to the completed state to ensure that
  /// any subsequent calls to wait() will fall through (until reset() is called of course).
  void wake() {
    VLOG(2) << "wake called";
    CHECK(!global_done);
    CHECK(count == 0) << "count == " << count;
    global_done = true;
    cores_in = cores();
    broadcast(&cv);
  }
  
public:
  
  GlobalCompletionEvent(): master_core(0) {}
  
  inline void set_shared_ptr(const void* p) { shared_arg = p; }
  template<typename T> inline const T* get_shared_ptr() { return reinterpret_cast<const T*>(shared_arg); }
  
  /// Must be called on all cores and finished before beginning any stealable work that syncs with this.
  /// Either call in SPMD (`on_all_cores()`) followed by `barrier()` or call `reset_all` from `user_main`
  /// or an equivalent task.
  void reset() {
    CHECK(cv.waiters_ == 0) << "resetting when tasks are still waiting";
    cores_in = cores();
    global_done = false;
    cancel_in_flight = false;
    enter_called = true;
//    barrier_done = false;
  }
  
  /// Called from user_main (or equivalent single controlling task). Trying to
  /// avoid having an explicit barrier in reset(), but still must guarantee that
  /// all cores have finished resetting before starting any public tasks that
  /// use the GCE to sync.
  void reset_all() {
    auto* gce = this;
    call_on_all_cores([gce]{
      gce->reset();
    });
  }
  
  /// Enroll more things that need to be completed before the global completion is, well, complete.
  /// This will send a cancel to the master if this core previously entered the cancellable barrier.
  void enroll(int64_t inc = 1) {
    count += inc;
    if (count == inc) { // 0 -> >=1
      send_cancel();
    }
  }
  
  /// Mark a certain number of things completed. When the global count on all cores goes to 0, all
  /// tasks waiting on the GCE will be woken.
  void complete(int64_t dec = 1) {
    CHECK(count >= dec) << "too many calls to complete(), count == " << count;
    
    count -= dec;
    VLOG(4) << "complete => " << count;
    
    if (count == 0) { // 1(+) -> 0
      send_enter();
    }
  }
  
  /// SPMD 
  void wait() {
    if (!global_done) {
      if (!cancel_in_flight && count == 0 && !enter_called) {
        // TODO: doing extra 'cancel_in_flight' check (?)
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

  
template<typename TF>
void privateTask(GlobalCompletionEvent * gce, TF tf) {
  gce->enroll();
  privateTask([gce,tf] {
    tf();
    gce->complete();
  });
}

template<GlobalCompletionEvent * GCE, typename TF>
void publicTask(TF tf) {
  GCE->enroll();
  Core origin = mycore();
  publicTask([origin,tf] {
    tf();
    complete(make_global(GCE,origin));
  });
}
  
}
