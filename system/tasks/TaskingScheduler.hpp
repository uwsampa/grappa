#ifndef _TASKING_SCHEDULER_HPP_
#define _TASKING_SCHEDULER_HPP_

#include "Scheduler.hpp"
#include "Communicator.hpp"
#include <Timestamp.hpp>
#include <glog/logging.h>
#include <sstream>

DECLARE_int64( periodic_poll_ticks );
DECLARE_bool(flush_on_idle);

extern void SoftXMT_idle_flush();

//extern int64_t num_active_tasks;
//extern int64_t task_calls;
//extern int64_t task_log_index;
//extern short active_task_log[1<<20];
//
//extern int64_t max_active;
//extern double  avg_active;

static inline double inc_avg(double curr_avg, uint64_t count, double val) {
	return curr_avg + (val-curr_avg)/(count);
}

class TaskingSchedulerStatistics {
  int64_t task_calls;
  int64_t task_log_index;
  short active_task_log[1<<20];
  
  int64_t max_active;
  double avg_active;
public:
  int64_t num_active_tasks;

  TaskingSchedulerStatistics() {
    reset();
  }
  
  void reset() {
    task_calls = 0;
    num_active_tasks = 0;
    task_log_index = 0;
    
    max_active = 0;
    avg_active = 0.0;
  }
  void print_active_task_log() {
#ifdef DEBUG
    if (task_log_index == 0) return;
    
    std::stringstream ss;
    for (int64_t i=0; i<task_log_index; i++) ss << active_task_log[i] << " ";
    LOG(INFO) << "Active tasks log: " << ss.str();
#endif
  }
  void sample() {
    task_calls++;
    if (num_active_tasks > max_active) max_active = num_active_tasks;
    avg_active = inc_avg(avg_active, task_calls, num_active_tasks);
#ifdef DEBUG  
    if ((task_calls % 1024) == 0) {
      active_task_log[task_log_index++] = num_active_tasks;
    }
#endif
  }
  void dump() {
    std::cout << "TaskStats { "
    << "max_active: " << max_active << ", "
    << "avg_active: " << avg_active << " }" << std::endl;
  }
};

class TaskManager;
struct task_worker_args;

class TaskingScheduler : public Scheduler {
    private:  
        ThreadQueue readyQ;
        ThreadQueue periodicQ;
        ThreadQueue unassignedQ;

        Thread * master;
        Thread * current_thread; 
        threadid_t nextId;
        
        uint64_t num_idle;
        TaskManager * task_manager;
        uint64_t num_workers;
        
        Thread * getWorker ();

        task_worker_args * work_args;

        // STUB: replace with real periodic threads
        SoftXMT_Timestamp previous_periodic_ts;
        Thread * periodicDequeue() {
            // tick the timestap counter
            SoftXMT_tick();
            SoftXMT_Timestamp current_ts = SoftXMT_get_timestamp();

            if( current_ts - previous_periodic_ts > FLAGS_periodic_poll_ticks ) {
                return periodicQ.dequeue();
            } else {
                return NULL;
            }
        }
        //////

        bool queuesFinished();

        Thread * nextCoroutine ( bool isBlocking=true ) {
            do {
                Thread * result;

                // check for periodic tasks
                result = periodicDequeue();
                if (result != NULL) {
                 //   DVLOG(5) << current_thread->id << " scheduler: pick periodic";
                    return result;
                }

                // check ready tasks
                result = readyQ.dequeue();
                if (result != NULL) {
                //    DVLOG(5) << current_thread->id << " scheduler: pick ready";
                    return result;
                }

                // check for new workers
                result = getWorker();
                if (result != NULL) {
                  //  DVLOG(5) << current_thread->id << " scheduler: pick task worker";
                    return result;
                }

                // no coroutines can run, so handle
                /*DVLOG(5) << current_thread->id << " scheduler: no coroutines can run"
                    << "[isBlocking=" << isBlocking
                    << " periodQ=" << (periodicQ.empty() ? "empty" : "full")
                    << " unassignedQ=" << (unassignedQ.empty() ? "empty" : "full") << "]";*/
                usleep(1);
            } while ( isBlocking || !queuesFinished() );
            // exit if all threads exited, including idle workers
            // TODO just as use mightBeWork as shortcut, also kill all idle unassigned workers on cbarrier_exit
            
            return NULL;
        }

        std::ostream& dump( std::ostream& o ) const {
            return o << "TaskingScheduler {" << std::endl
                << "  readyQ: " << readyQ << std::endl
                << "  periodicQ: " << periodicQ << std::endl
                << "  num_workers: " << num_workers << std::endl
                << "  num_idle: " << num_idle << std::endl
                << "  unassignedQ: " << unassignedQ << std::endl
                << "}";
        }

    public:
       TaskingSchedulerStatistics stats;
  
       TaskingScheduler ( Thread * master, TaskManager * taskman ); 

       Thread * get_current_thread() {
           return current_thread;
       }
       
       void assignTid( Thread * thr ) {
           thr->id = nextId++;
       }
        
      void createWorkers( uint64_t num );
      Thread* maybeSpawnCoroutines( );
      void onWorkerStart( );

       void unassigned( Thread * thr ) {
           unassignedQ.enqueue( thr );
       }

       void ready( Thread * thr ) {
            readyQ.enqueue( thr );
       }

       void periodic( Thread * thr ) {
           periodicQ.enqueue( thr );
           SoftXMT_tick();
           previous_periodic_ts = SoftXMT_get_timestamp();
       }
  
       void dump_stats() {
           stats.dump();
       }

       void reset_stats() {
         stats.reset();
       }
  
       /// run threads until all exit 
       void run ( );

       bool thread_yield( );
       bool thread_yield_periodic( );
       void thread_suspend( );
       void thread_wake( Thread * next );
       void thread_yield_wake( Thread * next );
       void thread_suspend_wake( Thread * next );
       bool thread_idle( uint64_t total_idle ); 
       bool thread_idle( );
       void thread_join( Thread* wait_on );

       // Start running threads from <scheduler> until one dies.  Return that Thread
       // (which can't be restarted--it's trash.)  If <result> non-NULL,
       // store the Thread's exit value there.
       // If no threads to run, returns NULL.
       Thread * thread_wait( void **result );


       void thread_on_exit( );

       friend std::ostream& operator<<( std::ostream& o, const TaskingScheduler& ts );
};  

struct task_worker_args {
    TaskManager *const tasks;
    TaskingScheduler *const scheduler;

    task_worker_args( TaskManager * task_manager, TaskingScheduler * sched )
        : tasks( task_manager )
        , scheduler( sched ) { }
};
       

/// Yield the CPU to the next Thread on your scheduler.  Doesn't ever touch
/// the master Thread.
inline bool TaskingScheduler::thread_yield( ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";

    ready( current_thread ); 
    
    Thread * yieldedThr = current_thread;

    Thread * next = nextCoroutine( );
    bool gotRescheduled = (next == yieldedThr);
    
    current_thread = next;
    //DVLOG(5) << "Thread " << yieldedThr->id << " yielding to " << next->id << (gotRescheduled ? " (same thread)." : " (diff thread).");
    thread_context_switch( yieldedThr, next, NULL);
    
    return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}

/// For periodic threads: yield the CPU to the next Thread on your scheduler.  Doesn't ever touch
/// the master Thread.
inline bool TaskingScheduler::thread_yield_periodic( ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";

    periodic( current_thread ); 
    
    Thread * yieldedThr = current_thread;

    Thread * next = nextCoroutine( );
    bool gotRescheduled = (next == yieldedThr);
    
    current_thread = next;
    //DVLOG(5) << "Thread " << yieldedThr->id << " yielding to " << next->id << (gotRescheduled ? " (same thread)." : " (diff thread).");
    thread_context_switch( yieldedThr, next, NULL);
    
    return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}

/// Suspend the current Thread. Thread is not placed on any queue.
inline void TaskingScheduler::thread_suspend( ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";
    CHECK( thread_is_running(current_thread) ) << "may only suspend a running coroutine";
    
    Thread * yieldedThr = current_thread;
    yieldedThr->co->running = 0; // XXX: hack; really want to know at a user Thread level that it isn't running
    Thread * next = nextCoroutine( );
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

/// Wake a suspended Thread by putting it on the run queue.
/// For now, waking a running Thread is a fatal error.
/// For now, waking a queued Thread is also a fatal error. 
/// For now, can only wake a Thread on your scheduler
inline void TaskingScheduler::thread_wake( Thread * next ) {
  CHECK( next->sched == this ) << "can only wake a Thread on your scheduler";
  CHECK( next->next == NULL ) << "woken Thread should not be on any queue";
  CHECK( !thread_is_running( next ) ) << "woken Thread should not be running";
    
  DVLOG(5) << "Thread " << current_thread->id << " wakes thread " << next->id;

  ready( next );
}

/// Yield the current Thread and wake a suspended thread.
/// For now, waking a running Thread is a fatal error.
/// For now, waking a queued Thread is also a fatal error. 
inline void TaskingScheduler::thread_yield_wake( Thread * next ) {    
    CHECK( current_thread != master ) << "can't yield on a system Thread";
    CHECK( next->sched == this ) << "can only wake a Thread on your scheduler";
    CHECK( next->next == NULL ) << "woken Thread should not be on any queue";
    CHECK( !thread_is_running( next ) ) << "woken Thread should not be running";
  
    Thread * yieldedThr = current_thread;
    ready( yieldedThr );
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

/// Suspend current Thread and wake a suspended thread.
/// For now, waking a running Thread is a fatal error.
/// For now, waking a queued Thread is also a fatal error. 
inline void TaskingScheduler::thread_suspend_wake( Thread *next ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";
    CHECK( next->next == NULL ) << "woken Thread should not be on any queue";
    CHECK( !thread_is_running( next ) ) << "woken Thread should not be running";
  
    Thread * yieldedThr = current_thread;
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

    
/// Make the current Thread idle.
/// Like suspend except the Thread is not blocking on a
/// particular resource, just waiting to be woken.
inline bool TaskingScheduler::thread_idle( uint64_t total_idle ) {
    CHECK( num_idle+1 <= total_idle ) << "number of idle threads should not be more than total"
                                      << " (" << num_idle+1 << " / " << total_idle << ")";
    if (num_idle+1 == total_idle) {
        DVLOG(5) << "going idle and is last";
        return false;
    } else {
        DVLOG(5) << "going idle and is " << num_idle+1 << " of " << total_idle;
        num_idle++;
        
        unassigned( current_thread );
        
        thread_suspend( );
        
        // woke so decrement idle counter
        num_idle--;

        return true;
    }
}

inline bool TaskingScheduler::thread_idle( ) {
    return thread_idle( num_workers );
}

inline void TaskingScheduler::thread_on_exit( ) {
  Thread * exitedThr = current_thread;
  current_thread = master;

  thread_context_switch( exitedThr, master, (void *)exitedThr);
}



#endif

