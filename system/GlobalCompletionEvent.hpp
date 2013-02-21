#include "Communicator.hpp"
#include "CompletionEvent.hpp"
#include "Tasking.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "Delegate.hpp"

namespace Grappa {

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
      auto* gce = this; // assume an identical GlobalCompletionEvent on each node (static globals)
      send_message(master_core, [gce]() {
        CHECK(mycore() == 0); // Core 0 is Master
        
        gce->cores_in++;
        VLOG(2) << "(completed) cores_in: " << gce->cores_in;
        
        if (gce->cores_in == cores()) {
          gce->barrier_done = true;
          VLOG(2) << "### done! ### cores_in: " << gce->cores_in;
          
          MessagePool<(1<<16)> pool;
          for (Core c=0; c<cores(); c++) {
            pool.send_message(c, [gce]() {
              gce->wake();
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
  
  /// Called from `reset_all`, not sufficient unless done on all cores before new completions are started
  void reset() {
    CHECK(cv.waiters_ == 0) << "resetting when tasks are still waiting";
    cores_in = 0;
    global_done = false;
    cancel_in_flight = false;
    enter_called = false;
    barrier_done = false;
  }
  
public:
  
  /// Called from user_main (or equivalent single controlling task). Trying to
  /// avoid having an explicit barrier in reset(), but still must guarantee that
  /// all cores have finished resetting before starting any public tasks that
  /// use the GCE to sync.
  void reset_all() {
    auto* gce = this;
    CompletionEvent ce(cores());
    MessagePool<(1<<16)> pool;
    for (Core c=0; c<cores(); c++) {
      pool.send_message(c,[&ce,gce]{
        gce->reset();
        send_heap_message(0, [&ce]{ ce.complete(); });
      });
    }
    ce.wait();
  }
  
  /// Enroll more things that need to be completed before the global completion is, well, complete.
  /// This will send a cancel to the master if this core previously entered the cancellable barrier.
  void enroll(int64_t inc = 1) {
    if (count == 0) { // 0 -> >=1
      send_cancel();
    }
    count += inc;
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
  
  /// SPMD or from user_main (maybe from some other task, but will sync globally so user_main makes the most sense)
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

}