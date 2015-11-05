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
#ifndef TASKING_SCHEDULER_HPP
#define TASKING_SCHEDULER_HPP

#include "Worker.hpp"
#include "ThreadQueue.hpp"
#include "Scheduler.hpp"
#include "Communicator.hpp"
#include <Timestamp.hpp>
#include <glog/logging.h>
#include <sstream>
#include "Metrics.hpp"
#include "HistogramMetric.hpp"

#include "Timestamp.hpp"
#include "PerformanceTools.hpp"
#include "MetricsTools.hpp"
#include "Metrics.hpp"


#ifdef VTRACE
#include <vt_user.h>
#endif

#include "StateTimer.hpp"

GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, scheduler_context_switches );
GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, scheduler_count);



// forward declarations
namespace Grappa {
namespace impl { void idle_flush_rdma_aggregator(); }
namespace Metrics { void sample_all(); }
}

// forward-declare old aggregator flush
bool idle_flush_aggregator();

DECLARE_int64( periodic_poll_ticks );
DECLARE_bool(poll_on_idle);
DECLARE_bool(flush_on_idle);
DECLARE_bool(rdma_flush_on_idle);

DECLARE_bool( stats_blob_enable );
DECLARE_string(stats_blob_filename);
DECLARE_int64( stats_blob_ticks );

namespace Grappa {

  namespace impl {

class TaskManager;
struct task_worker_args;

/// Worker scheduler that knows about Tasks.
/// Intended to schedule from a pool of worker threads that execute Tasks.
class TaskingScheduler : public Scheduler {
  private:  
    /// Queue for Threads that are ready to run
    PrefetchingThreadQueue readyQ;

    /// Queue for Threads that are to run periodically
    ThreadQueue periodicQ;

    /// Pool of idle workers that are not assigned to Tasks
    ThreadQueue unassignedQ;

    /// Master Worker that represents the main program thread
    Worker * master;

    /// Always points the the Worker that is currently running
    Worker * current_thread; 
    threadid_t nextId;

    /// number of idle workers
    uint64_t num_idle;

    /// number of workers assigned to Tasks
    uint64_t num_active_tasks;

    /// Max allowed active workers
    uint64_t max_allowed_active_workers;

    /// Reference to Task manager that is used by the scheduler
    /// for finding Tasks to assign to workers
    TaskManager * task_manager;

    /// total number of worker Threads
    uint64_t num_workers;

    /// Return an idle worker Worker
    Worker * getWorker ();

    task_worker_args * work_args;

    // STUB: replace with real periodic threads
    Grappa::Timestamp previous_periodic_ts;
    Grappa::Timestamp periodic_poll_ticks;
  inline bool should_run_periodic( Grappa::Timestamp current_ts ) {
    return current_ts - previous_periodic_ts > periodic_poll_ticks;
  }

    Worker * periodicDequeue(Grappa::Timestamp current_ts) {
      // // tick the timestap counter
      // Grappa::tick();
      // Grappa::Timestamp current_ts = Grappa::timestamp();

      if( should_run_periodic( current_ts ) ) {
        return periodicQ.dequeue();
      } else {
        return NULL;
      }
    }
    //////

    bool queuesFinished();

    /// make sure we don't context switch when we don't want to
    bool in_no_switch_region_;

    Grappa::Timestamp prev_ts;
    Grappa::Timestamp prev_stats_blob_ts;
    static const int64_t tick_scale = 1L; //(1L << 30);

    Worker * nextCoroutine ( bool isBlocking=true ) {
      scheduler_context_switches++;

      Grappa::Timestamp current_ts = 0;
#ifdef VTRACE_FULL
      VT_TRACER("nextCoroutine");
#endif
      CHECK_EQ( in_no_switch_region_, false ) << "Trying to context switch in no-switch region";

      do {
        Worker * result;
        scheduler_count++;

        // tick the timestap counter
        Grappa::tick();
        current_ts = Grappa::timestamp();

        // maybe sample
        if( Grappa::impl::take_tracing_sample ) {
          Grappa::impl::take_tracing_sample = false;
#ifdef HISTOGRAM_SAMPLED
          DVLOG(3) << "sampling histogram";
          Grappa::Metrics::histogram_sample();
#else
          Grappa::Metrics::sample();          
#endif
        }

        // if( ( global_communicator.mycore == 0 ) &&
        //     ( current_ts - prev_stats_blob_ts > FLAGS_stats_blob_ticks ) &&
        //     FLAGS_stats_blob_enable &&
        //     current_thread != master ) {
        //   prev_stats_blob_ts = current_ts;

        //   Grappa::Metrics::dump_stats_blob();
        // }

        // check for periodic tasks
        result = periodicDequeue(current_ts);
        if (result != NULL) {
          //   DVLOG(5) << current_thread->id << " scheduler: pick periodic";
          *(stats.state_timers[ stats.prev_state ]) += (current_ts - prev_ts) / tick_scale;
          stats.prev_state = TaskingSchedulerMetrics::StatePoll;
          prev_ts = current_ts;
          return result;
        }

        
        // check ready tasks
        result = readyQ.dequeue();
        if (result != NULL) {
          //    DVLOG(5) << current_thread->id << " scheduler: pick ready";
          *(stats.state_timers[ stats.prev_state ]) += (current_ts - prev_ts) / tick_scale;
          stats.prev_state = TaskingSchedulerMetrics::StateReady;
          prev_ts = current_ts;
          return result;
        }

        // check if scheduler is allowed to have more active workers
        if (num_active_tasks < max_allowed_active_workers) {
          // check for new workers
          result = getWorker();
          if (result != NULL) {
            //  DVLOG(5) << current_thread->id << " scheduler: pick task worker";
            *(stats.state_timers[ stats.prev_state ]) += (current_ts - prev_ts) / tick_scale;
            stats.prev_state = TaskingSchedulerMetrics::StateReady;
            prev_ts = current_ts;
            return result;
          }
        }

        
        if (FLAGS_poll_on_idle) {
          *(stats.state_timers[ stats.prev_state ]) += (current_ts - prev_ts) / tick_scale;

          if( FLAGS_rdma_flush_on_idle ) {
            Grappa::impl::idle_flush_rdma_aggregator();
          }

          if ( idle_flush_aggregator() ) {
            stats.prev_state = TaskingSchedulerMetrics::StateIdleUseful;
          } else {
            stats.prev_state = TaskingSchedulerMetrics::StateIdle;
          }


          StateTimer::enterState_scheduler();
        } else {
          *(stats.state_timers[ stats.prev_state ]) += (current_ts - prev_ts) / tick_scale;
          stats.prev_state = TaskingSchedulerMetrics::StateIdle;
          usleep(1);
        }

        prev_ts = current_ts;
        // no coroutines can run, so handle
        /*DVLOG(5) << current_thread->id << " scheduler: no coroutines can run"
          << "[isBlocking=" << isBlocking
          << " periodQ=" << (periodicQ.empty() ? "empty" : "full")
          << " unassignedQ=" << (unassignedQ.empty() ? "empty" : "full") << "]";*/
        //                usleep(1);
      } while ( isBlocking || !queuesFinished() );
      // exit if all threads exited, including idle workers
      // TODO just as use mightBeWork as shortcut, also kill all idle unassigned workers on cbarrier_exit
      return NULL;
    }

    /// Append a representation of the state of the scheduler to an output stream
    std::ostream& dump( std::ostream& o = std::cout, const char * terminator = "" ) const {
      return o << "\"TaskingScheduler\": {" << std::endl
        //<< "  \"hostname\": \"" << global_communicator.hostname() << "\"" << std::endl
        << "  \"pid\": " << getpid() << std::endl
        << "  \"readyQ\": " << readyQ << std::endl
        << "  \"periodicQ\": " << periodicQ << std::endl
        << "  \"num_workers\": " << num_workers << std::endl
        << "  \"num_idle\": " << num_idle << std::endl
        << "  \"unassignedQ\": " << unassignedQ << std::endl
        << "}";
    }

  public:
    /// Stats for the scheduler
    class TaskingSchedulerMetrics {
      private:
        int64_t task_log_index;
        short * active_task_log;

        TaskingScheduler * sched;

      public:
        enum State { StatePoll=0, StateReady=1, StateIdle=2, StateIdleUseful=3, StateLast=4 };
        SimpleMetric<uint64_t> * state_timers[ StateLast ];
        State prev_state;

        TaskingSchedulerMetrics();  // only for declarations that will be copy-assigned to

        /// Create new statistics tracking for scheduler
        TaskingSchedulerMetrics( TaskingScheduler * scheduler );
        
        ~TaskingSchedulerMetrics();

        void reset();

        void print_active_task_log();

        /// Take a sample of the scheduler state
        void sample();
    };

    TaskingSchedulerMetrics stats;

  void assign_time_to_networking() {
    stats.prev_state = TaskingSchedulerMetrics::StatePoll;
  }

    TaskingScheduler ( );
    void init ( Worker * master, TaskManager * taskman );

    void set_no_switch_region( bool val ) { in_no_switch_region_ = val; }
    bool in_no_switch_region() { return in_no_switch_region_; }

    /// Get the currently running Worker.
    Worker * get_current_thread() {
      return current_thread;
    }

    int64_t active_worker_count() {
      return this->num_active_tasks;
    }

    void shutdown_readyQ() {
      uint64_t count = 0;
      while ( readyQ.length() > 0 ) {
        Worker * w = readyQ.dequeue();
        //DVLOG(3) << "Worker found on readyQ at termination: " << *w;
        count++;
      }
      if ( count > 0 ) {
        DVLOG(2) <<    "Workers were found on readyQ at termination: " << count;
      } else {
        DVLOG(3) << "No Workers were found on readyQ at termination";
      }
    }

    /// Set allowed active workers to allow `n` more workers than are active now, or if '-1'
    /// is specified, allow all workers to be active.
    /// (this is mostly to make Core 0 with user_main not get forced to have fewer active)
    void allow_active_workers(int64_t n) {
      if (n == -1) {
        max_allowed_active_workers = num_workers;
      } else {
        //VLOG(1) << "mynode = " << global_communicator.mycore;
        max_allowed_active_workers = n + ((global_communicator.mycore == 0) ? 1 : 0);
      }
    }

    int64_t max_allowed_active() { return max_allowed_active_workers; }

    /// Assign the Worker a unique id for this scheduler
    void assignTid( Worker * thr ) {
      thr->id = nextId++;
    }

    void createWorkers( uint64_t num );
    Worker* maybeSpawnCoroutines( );
    void onWorkerStart( );

    uint64_t active_task_count() {
      return num_active_tasks;
    }

    /// Mark the Worker as an idle worker
    void unassigned( Worker * thr ) {
      unassignedQ.enqueue( thr );
    }

    /// Mark the Worker as ready to run
    void ready( Worker * thr ) {
      readyQ.enqueue( thr );
    }

    /// Put the Worker into the periodic queue
    void periodic( Worker * thr ) {
      periodicQ.enqueue( thr );
      Grappa::tick();
      Grappa::Timestamp current_ts = Grappa::force_tick();
      previous_periodic_ts = current_ts;
    }

    /// Reset scheduler statistics
    void reset_stats() {
      stats.reset();
    }

    /// run threads until all exit 
    void run ( );

    bool thread_maybe_yield( );
    bool thread_yield( );
    bool thread_yield_periodic( );
    void thread_suspend( );
    void thread_wake( Worker * next );
  //void thread_yield_wake( Worker * next );
  //void thread_suspend_wake( Worker * next );
    bool thread_idle( uint64_t total_idle ); 
    bool thread_idle( );

    Worker * thread_wait( void **result );


    void thread_on_exit( );

    friend std::ostream& operator<<( std::ostream& o, const TaskingScheduler& ts );
    friend void workerLoop ( Worker *, void * );
};  

/// Arguments to workerLoop() worker Worker routine
struct task_worker_args {
  TaskManager *const tasks;
  TaskingScheduler *const scheduler;

  task_worker_args( TaskManager * task_manager, TaskingScheduler * sched )
    : tasks( task_manager )
      , scheduler( sched ) { }
};

/// Check the timestamp counter to see if it's time to run the periodic queue
inline bool TaskingScheduler::thread_maybe_yield( ) {
  bool yielded = false;

  // tick the timestap counter
  Grappa::Timestamp current_ts = Grappa::force_tick();
  
  if( should_run_periodic( current_ts ) ) {
    yielded = true;
    thread_yield();
  }

  return yielded;
}



/// Yield the CPU to the next Worker on this scheduler. 
/// Cannot be called during the master Worker.
inline bool TaskingScheduler::thread_yield( ) {
  CHECK( current_thread != master ) << "can't yield on a system Worker";
  StateTimer::enterState_scheduler();

  ready( current_thread ); 

  Worker * yieldedThr = current_thread;

  Worker * next = nextCoroutine( );
  bool gotRescheduled = (next == yieldedThr);

  current_thread = next;
  //DVLOG(5) << "Worker " << yieldedThr->id << " yielding to " << next->id << (gotRescheduled ? " (same thread)." : " (diff thread).");
  thread_context_switch( yieldedThr, next, NULL);

  return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}

/// For periodic threads: yield the CPU to the next Worker on this scheduler.  
/// Cannot be called during the master Worker.
inline bool TaskingScheduler::thread_yield_periodic( ) {
  CHECK( current_thread != master ) << "can't yield on a system Worker";
  StateTimer::enterState_scheduler();

  periodic( current_thread ); 

  Worker * yieldedThr = current_thread;

  Worker * next = nextCoroutine( );
  bool gotRescheduled = (next == yieldedThr);

  current_thread = next;
  //DVLOG(5) << "Worker " << yieldedThr->id << " yielding to " << next->id << (gotRescheduled ? " (same thread)." : " (diff thread).");
  thread_context_switch( yieldedThr, next, NULL);

  return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}

/// Suspend the current Worker. Worker is not placed on any queue.
/// Cannot be called during the master Worker.
inline void TaskingScheduler::thread_suspend( ) {
  CHECK( current_thread != master ) << "can't yield on a system Worker";
  CHECK( current_thread->running ) << "may only suspend a running coroutine";
  StateTimer::enterState_scheduler();

  Worker * yieldedThr = current_thread;
  yieldedThr->running = 0; // XXX: hack; really want to know at a user Worker level that it isn't running
  yieldedThr->suspended = 1;
  Worker * next = nextCoroutine( );

  current_thread = next;
  thread_context_switch( yieldedThr, next, NULL);
}

/// Wake a suspended Worker by putting it on the run queue.
/// For now, waking a running Worker is a fatal error.
/// For now, waking a queued Worker is also a fatal error. 
/// For now, can only wake a Worker on your scheduler
inline void TaskingScheduler::thread_wake( Worker * next ) {
  CHECK( next->sched == static_cast<Scheduler*>(this) ) << "can only wake a Worker on your scheduler (next="<<(void*) next << " next->sched="<<(void*)next->sched <<" this="<<(void*)this;
  CHECK( next->next == NULL ) << "woken Worker should not be on any queue";
  CHECK( !next->running ) << "woken Worker should not be running";

  next->suspended = 0;

  DVLOG(5) << "Worker " << current_thread->id << " wakes thread " << next->id;

  ready( next );
}

// /// Yield the current Worker and wake a suspended thread.
// /// For now, waking a running Worker is a fatal error.
// /// For now, waking a queued Worker is also a fatal error. 
// inline void TaskingScheduler::thread_yield_wake( Worker * next ) {    
//   CHECK( current_thread != master ) << "can't yield on a system Worker";
//   CHECK( next->sched == static_cast<Scheduler*>(this) ) << "can only wake a Worker on your scheduler";
//   CHECK( next->next == NULL ) << "woken Worker should not be on any queue";
//   CHECK( !next->running ) << "woken Worker should not be running";
//   StateTimer::enterState_scheduler();

//   next->suspended = 0;

//   Worker * yieldedThr = current_thread;
//   ready( yieldedThr );

//   current_thread = next;
//   thread_context_switch( yieldedThr, next, NULL);
// }

// /// Suspend current Worker and wake a suspended thread.
// /// For now, waking a running Worker is a fatal error.
// /// For now, waking a queued Worker is also a fatal error. 
// inline void TaskingScheduler::thread_suspend_wake( Worker *next ) {
//   CHECK( current_thread != master ) << "can't yield on a system Worker";
//   CHECK( next->next == NULL ) << "woken Worker should not be on any queue";
//   CHECK( !next->running ) << "woken Worker should not be running";
//   StateTimer::enterState_scheduler();

//   next->suspended = 0;

//   Worker * yieldedThr = current_thread;
//   yieldedThr->suspended = 1;

//   current_thread = next;
//   thread_context_switch( yieldedThr, next, NULL);
// }


/// Make the current Worker idle.
/// Like suspend except the Worker is not blocking on a
/// particular resource, just waiting to be woken.
/// @param total_idle the total number of possible idle threads
inline bool TaskingScheduler::thread_idle( uint64_t total_idle ) {
  CHECK( num_idle+1 <= total_idle ) << "number of idle threads should not be more than total"
    << " (" << num_idle+1 << " / " << total_idle << ")";
  if (num_idle+1 == total_idle) {
    DVLOG(5) << "going idle and is last";
  } else {
    DVLOG(5) << "going idle and is " << num_idle+1 << " of " << total_idle;
  }

  num_idle++;
  current_thread->idle = 1;

  unassigned( current_thread );

  thread_suspend( );

  // woke so decrement idle counter
  num_idle--;
  current_thread->idle = 0;

  return true;
}

/// See bool TaskingScheduler::thread_idle(uint64_t), defaults
/// to total being the number of scheduler workers.
inline bool TaskingScheduler::thread_idle( ) {
  return thread_idle( num_workers );
}

/// Callback for the end of a Worker
inline void TaskingScheduler::thread_on_exit( ) {
  Worker * exitedThr = current_thread;
  current_thread = master;

  thread_context_switch( exitedThr, master, (void *)exitedThr);
}

/// instance
extern TaskingScheduler global_scheduler;

} // namespace impl


///////////////////////////
/// Helpers
///
/// @addtogroup Delegates
/// @{


inline Worker* current_worker() {
  return impl::global_scheduler.get_current_thread();
}

/// Yield to scheduler, placing current Worker on run queue.
static inline void yield() { impl::global_scheduler.thread_yield(); }

/// Yield to scheduler, placing current Worker on periodic queue.
static inline void yield_periodic() { impl::global_scheduler.thread_yield_periodic( ); }

/// Yield to scheduler, suspending current Worker.
static inline void suspend() {
  DVLOG(5) << "suspending Worker " << impl::global_scheduler.get_current_thread() << "(# " << impl::global_scheduler.get_current_thread()->id << ")";
  impl::global_scheduler.thread_suspend( );
  //CHECK_EQ(retval, 0) << "Worker " << th1 << " suspension failed. Have the server threads exited?";
}

/// Wake a Worker by putting it on the run queue, leaving the current thread running.
static inline void wake( Worker * t ) {
  DVLOG(5) << impl::global_scheduler.get_current_thread()->id << " waking Worker " << t;
  impl::global_scheduler.thread_wake( t );
}

// /// Wake a Worker t by placing current thread on run queue and running t next.
// static inline void yield_wake( Worker * t ) {
//   DVLOG(5) << "yielding Worker " << impl::global_scheduler.get_current_thread() << " and waking thread " << t;
//   impl::global_scheduler.thread_yield_wake( t );
// }

// /// Wake a Worker t by suspending current thread and running t next.
// static inline void suspend_wake( Worker * t ) {
//   DVLOG(5) << "suspending Worker " << impl::global_scheduler.get_current_thread() << " and waking thread " << t;
//   impl::global_scheduler.thread_suspend_wake( t );
// }

/// Place current thread on queue to be reused by tasking layer as a worker.
/// @deprecated should not be in the public API because it is a Worker-level not Task-level routine
static inline bool thread_idle() {
  DVLOG(5) << "Worker " << impl::global_scheduler.get_current_thread()->id << " going idle";
  return impl::global_scheduler.thread_idle();
}

/// @}

} // namespace Grappa

#endif // TASKING_SCHEDULER_HPP
