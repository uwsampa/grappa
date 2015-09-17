////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "Communicator.hpp"
#include "CompletionEvent.hpp"
#include "Tasking.hpp"
#include "Message.hpp"
#include "DelegateBase.hpp"
#include "Collective.hpp"
#include "Timestamp.hpp"
#include <type_traits>
#include <vector>
#include "Metrics.hpp"

#define PRINT_MSG(m) "msg(" << &(m) << ", src:" << (m).source_ << ", dst:" << (m).destination_ << ", enq:" << (m).is_enqueued_ << ", sent:" << (m).is_sent_ << ", deliv:" << (m).is_delivered_ << ")"

DECLARE_bool( flatten_completions );
DECLARE_bool( enable_aggregation );

/// total number of times "complete" has to be called on another core
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, gce_total_remote_completions);

/// actual number of completion messages we send (less with flattening)
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, gce_completions_sent);

namespace Grappa {
/// @addtogroup Synchronization
/// @{

/// Type returned by enroll used to indicate where to send a completion
struct CompletionTarget {
  Core core;
};

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

  // Master barrier only
  Core cores_out;
  
  // after completion, re-enroll this many tasks on master core.
  int reenroll_count;
  
  // temporary storage for blocked threads while exiting from wake
  ConditionVariable temporary_waking_cv;
  Core temporary_waking_cores_out;

  /// pointer to shared arg for loops that use a GCE
  const void * shared_ptr;

  static std::vector<GlobalCompletionEvent*> user_tracked_gces;
  
  struct DoComplete {
    GlobalCompletionEvent * gce;
    int64_t dec;
    
    DoComplete(): gce(nullptr), dec(0) {}
    
    void operator()() {
      gce->complete(dec);
    }
  };
  
  
  class CompletionMessage: public Message<DoComplete> {    
  public:
    Core target;
    int64_t completes_to_send;
    CompletionMessage(Core target = -1): Message(), target(target), completes_to_send(0) {}
    
    bool waiting_to_send() {
      return this->is_enqueued_ && !this->is_sent_;
    }
    
    virtual void mark_sent() {
      Core dest = this->destination_;
      DVLOG(5) << "marking sent " << PRINT_MSG(*this);
      Message<DoComplete>::mark_sent();
      
      if (Grappa::mycore() == this->source_) {
        this->reset();
        if (completes_to_send > 0) {
          DVLOG(5) << "re-sending -- " << completes_to_send << " to Core[" << dest << "] " << PRINT_MSG(*this);
          (*this)->dec = completes_to_send;
          completes_to_send = 0;
          auto prev = Grappa::impl::global_scheduler.in_no_switch_region();
          Grappa::impl::global_scheduler.set_no_switch_region( true );
          this->enqueue(target);
          Grappa::impl::global_scheduler.set_no_switch_region( prev );
        }
      }
    }
    
    virtual const size_t size() const { return sizeof(*this); }
  } __attribute__((aligned(64)));
  
  
  CompletionMessage * completion_msgs;
  
  CompletionMessage& get_completion_msg(Core c) {
    if (completion_msgs == nullptr) { init_completion_msgs(); }
    return completion_msgs[c];
  }
  
  inline void init_completion_msgs() {
    completion_msgs = locale_alloc<CompletionMessage>(cores());
  
    for (Core c=0; c<cores(); c++) {
      new (completion_msgs+c) CompletionMessage(c); // call constructor!
      completion_msgs[c]->gce = this; // (pointer must be the same on all cores)
    }
  }
  
public:
  
  /// The GlobalCompletionEvents master core is defined to be core 0.
  static const Core master_core;

  static std::vector<GlobalCompletionEvent*> get_user_tracked();

  int64_t incomplete() const { return count; }
  
  /// Send a completion message to the originating core. Uses the local instance of the gce to
  /// keep track of information in order to flatten completions automatically.
  void send_completion(Core owner, int64_t dec = 1) {
    if (owner == mycore()) {
      VLOG(5) << "complete locally ";
      this->complete(dec);
    } else {
      gce_total_remote_completions++;
      auto& cm = get_completion_msg(owner);
      if (cm.waiting_to_send()) {
        cm.completes_to_send++;
        DVLOG(5) << "flattening completion to Core[" << owner << "] (currently at " << cm.completes_to_send << ")";
      } else {
        CHECK_EQ(cm.completes_to_send, 0) << "why haven't we sent these already? cm(" << &cm << ", is_sent: " << cm.is_sent_ << ", is_enqueued: " << cm.is_enqueued_ << ", dest: " << cm.destination_ << ")";
        DVLOG(5) << "sending " << dec << " to Core[" << owner << "]";
        cm->dec = dec;
        gce_completions_sent++;
        cm.enqueue(owner);
      }
    }
  }
  
  GlobalCompletionEvent(bool user_track=false): reenroll_count(0), temporary_waking_cv(), temporary_waking_cores_out(0), completion_msgs(nullptr) {
    reset();

    if (user_track) {
      user_tracked_gces.push_back(this);
    } 
  }

  ~GlobalCompletionEvent() {
    if (completion_msgs != nullptr) locale_free(completion_msgs);
  }
  
  inline void set_shared_ptr(const void* p) { shared_ptr = p; }
  template<typename T> inline const T* get_shared_ptr() { return reinterpret_cast<const T*>(shared_ptr); }
  
  /// This is the state the GCE's on each core should be in at the beginning, and the 
  /// state they should end up in after a Completion phase. It should no longer be necessary
  /// to call `reset` between calls to `wait` as long as nothing fishy is going on.
  void reset() {
    count = 0;
    cv.waiters_ = 0;
    cores_out = 0;
    reenroll_count = 0;
    event_in_progress = false;
  }
  
  /// Enroll more things that need to be completed before the global completion is, well, complete.
  /// This will send a cancel to the master if this core previously entered the cancellable barrier.
  ///
  /// Blocks until cancel completes (if it must cancel) to ensure correct ordering, therefore
  /// cannot be called from message handler.
  CompletionTarget enroll(int64_t inc = 1) {
    if (inc == 0) return {mycore()};
    
    CHECK_GE(inc, 1);
    count += inc;
    DVLOG(5) << "enroll " << inc << " -> " << count << " gce("<<this<<")";
    
    // first one to have work here
    if (count == inc) { // count[0 -> inc]
      event_in_progress = true; // optimization to save checking in wait()
      // cancel barrier
      Core co = impl::call(master_core, [this] {
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
      DVLOG(2) << "gce(" << this << " cores_out: " << co << ", count: " << count << ")";
    }

    return {mycore()};
  }

  /// Enqueue tasks to GCE which will be automatically re-enrolled when the current phase completes.
  /// This must be called on the master core.
  CompletionTarget enroll_recurring(int64_t inc = 1) {
    CHECK_EQ(mycore(), master_core) << "Enroll count must be positive.";
    reenroll_count = inc;
    return enroll(inc);
  }

  /// Mark a certain number of things completed. When the global count on all cores goes to 0, all
  /// tasks waiting on the GCE will be woken.
  ///
  /// Note: this can be called in a message handler (e.g. remote completes from stolen tasks).
  void complete(int64_t dec = 1) {
    count -= dec;
    DVLOG(4) << "core " << mycore() << " complete (" << count << ") -- gce(" << this << ") cores_out: " << cores_out;
    
    // out of work here
    if (count == 0) { // count[dec -> 0]
      // enter cancellable barrier
      send_heap_message(master_core, [this] {
        cores_out--;
        DVLOG(4) << "core entered barrier (cores_out:"<< cores_out <<")";
        
        // if all are in
        if (cores_out == 0) { // cores_out[1 -> 0]
          CHECK_EQ(count, 0);

          // first, go reset everyone
          temporary_waking_cores_out = cores();
          auto re = this->reenroll_count; // remember if we requested reenroll
          for (Core c = 0; c < cores(); c++) {
            send_heap_message(c, [this,re] {
              CHECK_EQ(count, 0);
              temporary_waking_cv = cv; // capture current list of waiters
              reset(); // reset, now anyone else calling `wait` should fall through
              DVLOG(3) << "reset";
              
              // if requested, remind cores that we're re-enrolling at the master core
              if( re ) {
                DVLOG(5) << "Setting event_in_progress";
                event_in_progress = true;
              }

              // then, once everyone is reset,
              send_heap_message(master_core, [this,re] {
                temporary_waking_cores_out--;
                if (temporary_waking_cores_out == 0) {
                  
                  // if requested, re-enroll cores
                  if( re ) {
                    DVLOG(5) << "Setting count (" << count << ") to " << re << " with event_in_progress " << event_in_progress;
                    count = re;          // expect this many completions
                    cores_out = 1;       // remember that the master core has outstanding tasks
                    reenroll_count = re; // remember to re-enroll this many cores/tasks next time
                  }

                  // notify everyone to wake
                  for (Core c = 0; c < cores(); c++) {
                    send_heap_message(c, [this] {
                      DVLOG(3) << "broadcast";
                      broadcast(&temporary_waking_cv); // wake anyone who was waiting here
                      temporary_waking_cv.waiters_ = 0; 
                    });
                  }
                }
              });
            });
          }
        }
      });
    }
  }

  /// Wrapper to call combining completion instead of local completion
  /// when given a CompletionTarget.
  inline void complete(CompletionTarget ct, int64_t decr = 1) {
    DVLOG(5) << "called remote complete";
    if (FLAGS_flatten_completions) {
      send_completion(ct.core, decr);
    } else {
      if (ct.core == mycore()) {
        complete(decr);
      } else {
        if (decr == 1) {
          // (common case) don't send full 8 bytes just to decrement by 1
          send_heap_message(ct.core, [this] {
            complete();
          });
        } else {
          send_heap_message(ct.core, [this,decr] {
            complete(decr);
          });
        }
      }
    }
  }
  
  /// Wrapper to call combining completion instead of local completion
  /// when given a GCE global address.
  inline void complete(GlobalAddress<GlobalCompletionEvent> ce, int64_t decr = 1) {
    return complete({ce.core()}, decr);
  }
  
  /// Suspend calling task until all tasks completed (including additional tasks enrolled 
  /// before count goes to 0). This can be called from as many tasks as desired on
  /// different cores, as long as it is ensured that a task in the desired phase has been
  /// enrolled before wait is called.
  ///
  /// If no tasks have been enrolled, or all have been completed by the time `wait` is called,
  /// this will fall through and not suspend the calling task.
  void wait() {
    DVLOG(3) << "wait(): gce(" << this << " event_in_progress: " << event_in_progress << ", count: " << count << ")";
    if (event_in_progress) {
      Grappa::wait(&cv);
    } else {
      // conservative check, in case we're calling `wait` without calling `enroll`
      if (impl::call(master_core, [this]{ return cores_out; }) > 0) {
//      if (impl::call(master_core, [this]{ return event_in_progress; })) {
        Grappa::wait(&cv);
        DVLOG(3) << "woke from conservative check";
      }
      DVLOG(3) << "fell thru conservative check";
    }
  }

};

inline CompletionTarget enroll(GlobalAddress<GlobalCompletionEvent> ce, int64_t decr = 1) {
  return ce.pointer()->enroll(decr);
}

/// Allow calling send_completion using the old way (with global address)
/// TODO: replace all instances with gce.send_completion and remove this?
inline void complete(GlobalAddress<GlobalCompletionEvent> ce, int64_t decr = 1) {
  ce.pointer()->complete(ce, decr);
}

/// @}
} // namespace Grappa

/// Synchronizing spawns
namespace Grappa {
  /// @addtogroup Tasking
  /// @{
  
  /// Synchronizing private task spawn. Automatically enrolls task with GlobalCompletionEvent and
  /// does local `complete` when done (if GCE is non-null).
  /// In this version, GCE is a template parameter to avoid taking up space in the task's args.
  template< TaskMode B,
            GlobalCompletionEvent * C,
            typename TF = decltype(nullptr) >
  void spawn(TF tf) {
    if (C) C->enroll();
    Core origin = mycore();
    spawn<B>([tf,origin] {
      tf();
      if (C) C->send_completion(origin);
    });
  }
  
  ///@}
  
  /// Overload
  template< GlobalCompletionEvent * C,
            TaskMode B = TaskMode::Bound,
            typename TF = decltype(nullptr) >
  void spawn(TF tf) { spawn<B,C>(tf); }
  
  namespace impl {
    extern GlobalCompletionEvent local_gce;
  }
  
  template< GlobalCompletionEvent * C = &impl::local_gce,
            typename F = decltype(nullptr) >
  void finish(F f) {
    C->enroll();
    f();
    C->complete();
    C->wait();
  }
  
} // namespace Grappa

