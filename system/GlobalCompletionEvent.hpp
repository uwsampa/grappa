#pragma once

#include "Communicator.hpp"
#include "CompletionEvent.hpp"
#include "Tasking.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "Delegate.hpp"
#include "Collective.hpp"
#include "Timestamp.hpp"
#include <type_traits>

namespace Grappa {

/// GlobalCompletionEvent (GCE):
/// Synchronization construct for determining when a global phase of asynchronous tasks have 
/// all completed. For example, can be used to ensure that all tasks of a parallel loop have
/// completed, including tasks or asynchronous delegates spawned during execution of the
/// parallel loop.
///
/// A GlobalCompletionEvent must be replicated at the same address on all cores (the easiest
/// way is to declare it as file-global or static storage). TODO: allow dynamic allocation 
/// in Grappa linear address space.
///
/// Anyone calling "wait" after at least some work has been "enrolled" globally will block.
/// When *all* work has been completed on all cores, all tasks waiting on the GCE will be woken.
///
/// To ensure consistent behavior and guarantee deadlock freedom, every "enroll" must be 
/// causally followed by "complete", so either must be completed from the same task that
/// called "enroll" or from a task spawned (transitively) from the enrolling task.
///
/// This is meant to be a reusable barrier; GCE's should begin in a state where a call to
/// `wait` will fall through, and end up, after all tasks are completed, back in the same
/// state. Note: `reset` no longer needs to be called between phases. Instead, it just
/// must be guaranteed that at least one task has been enrolled before anyone tries to
/// call `wait` otherwise they may fall through before the enrollment has completed.
class GlobalCompletionEvent : public CompletionEvent {
  // All nodes
  // (count)
  // (cv)
  bool event_in_progress;
  Core master_core;
  
  // Master barrier only
  Core cores_out;
  
  /// pointer to shared arg for loops that use a GCE
  const void * shared_ptr;
  
public:
  
  GlobalCompletionEvent(): master_core(0) { reset(); }
  
  inline void set_shared_ptr(const void* p) { shared_ptr = p; }
  template<typename T> inline const T* get_shared_ptr() { return reinterpret_cast<const T*>(shared_ptr); }
  
  /// This is the state the GCE's on each core should be in at the beginning, and the 
  /// state they should end up in after a Completion phase. I should no longer be necessary
  /// to call `reset` between calls to `wait` as long as nothing fishy is going on.
  void reset() {
    count = 0;
    cv.waiters_ = 0;
    cores_out = 0;
    event_in_progress = false;
  }
  
  /// Enroll more things that need to be completed before the global completion is, well, complete.
  /// This will send a cancel to the master if this core previously entered the cancellable barrier.
  ///
  /// Blocks until cancel completes (if it must cancel) to ensure correct ordering, therefore
  /// cannot be called from message handler.
  void enroll(int64_t inc = 1) {
    if (inc == 0) return;
    
    CHECK_GE(inc, 1);
    count += inc;
    
    // first one to have work here
    if (count == inc) { // count[0 -> inc]
      event_in_progress = true; // optimization to save checking in wait()
      // cancel barrier
      Core co = delegate::call(master_core, [this] {
        cores_out++;
        return cores_out;
      });
      // first one to cancel barrier should make sure other cores are ready to wait
      if (co == 1) { // cores_out[0 -> 1]
        event_in_progress = true;
        call_on_all_cores([this] {
          event_in_progress = true;
        });
        CHECK(event_in_progress);
      }
      // block until cancelled
      CHECK_GT(count, 0);
      VLOG(2) << "cores_out: " << co << ", count: " << count;
    }
  }
  
  /// Mark a certain number of things completed. When the global count on all cores goes to 0, all
  /// tasks waiting on the GCE will be woken.
  ///
  /// Note: this can be called in a message handler (e.g. remote completes from stolen tasks).
  void complete(int64_t dec = 1) {
    count -= dec;
    DVLOG(4) << "complete (" << count << ") -- " << this;
    
    // out of work here
    if (count == 0) { // count[dec -> 0]
      // enter cancellable barrier
      send_heap_message(master_core, [this] {
        cores_out--;
        
        // if all are in
        if (cores_out == 0) { // cores_out[1 -> 0]
          CHECK_EQ(count, 0);
          // notify everyone to wake
          for (Core c = 0; c < cores(); c++) {
            send_heap_message(c, [this] {
              CHECK_EQ(count, 0);
              broadcast(&cv); // wake anyone who was waiting here
              reset(); // reset, now anyone else calling `wait` should fall through
            });
          }
        }
      });
    }
  }
  
  /// Suspend calling task until all tasks completed (including additional tasks enrolled 
  /// before count goes to 0). This can be called from as many tasks as desired on
  /// different cores, as long as it is ensured that a task in the desired phase has been
  /// enrolled before wait is called.
  ///
  /// If no tasks have been enrolled, or all have been completed by the time `wait` is called,
  /// this will fall through and not suspend the calling task.
  void wait() {
    VLOG(3) << "wait(): event_in_progress: " << event_in_progress << ", count: " << count;
    if (event_in_progress) {
      Grappa::wait(&cv);
    } else {
      // conservative check, in case we're calling `wait` without calling `enroll`
      if (delegate::call(master_core, [this]{ return cores_out; }) > 0) {
//      if (delegate::call(master_core, [this]{ return event_in_progress; })) {
        Grappa::wait(&cv);
        VLOG(3) << "woke from conservative check";
      }
      VLOG(3) << "fell thru conservative check";
    }
    CHECK(!event_in_progress);
    CHECK_EQ(count, 0);
  }
  
};

} // namespace Grappa

/// Synchronizing delegates
namespace Grappa {
  /// Synchronizing private task spawn. Automatically enrolls task with GlobalCompletionEvent and
  /// does local `complete` when done (if GCE is non-null).
  template<typename TF>
  void privateTask(GlobalCompletionEvent * gce, TF tf) {
    gce->enroll();
    privateTask([gce,tf] {
      tf();
      gce->complete();
    });
  }

  /// Synchronizing private task spawn. Automatically enrolls task with GlobalCompletionEvent and
  /// does local `complete` when done (if GCE is non-null).
  /// In this version, GCE is a template parameter to avoid taking up space in the task's args.
  template<GlobalCompletionEvent * GCE, typename TF>
  void privateTask(TF tf) {
    if (GCE) GCE->enroll();
    Core origin = mycore();
    privateTask([tf] {
      tf();
      if (GCE) GCE->complete();
    });
  }

  /// Synchronizing public task spawn. Automatically enrolls task with GlobalCompletionEvent and
  /// sends `complete`  message when done (if GCE is non-null).
  template<GlobalCompletionEvent * GCE, typename TF>
  inline void publicTask(TF tf) {
    if (GCE) GCE->enroll();
    Core origin = mycore();
    publicTask([origin,tf] {
      tf();
      if (GCE) complete(make_global(GCE,origin));
    });
  }
  
} // namespace Grappa
