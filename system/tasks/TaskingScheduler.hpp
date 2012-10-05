#ifndef _TASKING_SCHEDULER_HPP_
#define _TASKING_SCHEDULER_HPP_

#include "Scheduler.hpp"
#include "Communicator.hpp"
#include "Aggregator.hpp"
#include <Timestamp.hpp>
#include <glog/logging.h>
#include <sstream>

#include "Timestamp.hpp"
#include "PerformanceTools.hpp"
#include "StatisticsTools.hpp"
#ifdef VTRACE
#include <vt_user.h>
#endif

#include "StateTimer.hpp"


// maybe sample
extern bool take_profiling_sample;
void SoftXMT_take_profiling_sample();

DECLARE_int64( periodic_poll_ticks );
DECLARE_bool(flush_on_idle);


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
        uint64_t num_active_tasks;
        TaskManager * task_manager;
        uint64_t num_workers;
        
        Thread * getWorker ();

        task_worker_args * work_args;

        // STUB: replace with real periodic threads
        SoftXMT_Timestamp previous_periodic_ts;
        Thread * periodicDequeue(SoftXMT_Timestamp current_ts) {
            // // tick the timestap counter
            // SoftXMT_tick();
            // SoftXMT_Timestamp current_ts = SoftXMT_get_timestamp();

            if( current_ts - previous_periodic_ts > FLAGS_periodic_poll_ticks ) {
                return periodicQ.dequeue();
            } else {
                return NULL;
            }
        }
        //////

        bool queuesFinished();
  
  SoftXMT_Timestamp prev_ts;
  static const int64_t tick_scale = 1L; //(1L << 30);

        Thread * nextCoroutine ( bool isBlocking=true ) {
	  SoftXMT_Timestamp current_ts = 0;
#ifdef VTRACE_FULL
	  VT_TRACER("nextCoroutine");
#endif
            do {
                Thread * result;
		++stats.scheduler_count;

		// tick the timestap counter
		SoftXMT_tick();
		current_ts = SoftXMT_get_timestamp();

		// maybe sample
		if( take_profiling_sample ) {
		  take_profiling_sample = false;
		  SoftXMT_take_profiling_sample();
		}

                // check for periodic tasks
                result = periodicDequeue(current_ts);
                if (result != NULL) {
                 //   DVLOG(5) << current_thread->id << " scheduler: pick periodic";
		  stats.state_timers[ stats.prev_state ] += (current_ts - prev_ts) / tick_scale;
		  stats.prev_state = TaskingSchedulerStatistics::StatePoll;
		prev_ts = current_ts;
                    return result;
                }

                // check ready tasks
                result = readyQ.dequeue();
                if (result != NULL) {
                  readyQ.prefetch();
                //    DVLOG(5) << current_thread->id << " scheduler: pick ready";
		  stats.state_timers[ stats.prev_state ] += (current_ts - prev_ts) / tick_scale;
		  stats.prev_state = TaskingSchedulerStatistics::StateReady;
		prev_ts = current_ts;
                    return result;
                }

                // check for new workers
                result = getWorker();
                if (result != NULL) {
                  //  DVLOG(5) << current_thread->id << " scheduler: pick task worker";
		  stats.state_timers[ stats.prev_state ] += (current_ts - prev_ts) / tick_scale;
		  stats.prev_state = TaskingSchedulerStatistics::StateReady;
		  prev_ts = current_ts;
                    return result;
                }

                if (FLAGS_flush_on_idle) {
		  stats.state_timers[ stats.prev_state ] += (current_ts - prev_ts) / tick_scale;
                  if ( global_aggregator.idle_flush_poll() ) {
		                stats.prev_state = TaskingSchedulerStatistics::StateIdleUseful;
                  } else {
                    stats.prev_state = TaskingSchedulerStatistics::StateIdle;
                  }
                  
                  StateTimer::enterState_scheduler();
                } else {
		  stats.state_timers[ stats.prev_state ] += (current_ts - prev_ts) / tick_scale;
		  stats.prev_state = TaskingSchedulerStatistics::StateIdle;
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
        class TaskingSchedulerStatistics {
            private:
                int64_t task_calls;
                int64_t task_log_index;
                short * active_task_log;

                uint64_t max_active;
                double avg_active;
                double avg_ready;

#ifdef VTRACE_SAMPLED
	  unsigned tasking_scheduler_grp_vt;
	  unsigned active_tasks_out_ev_vt;
	  unsigned num_idle_out_ev_vt;
	  unsigned readyQ_size_ev_vt;
#endif


                TaskingScheduler * sched;
                
                unsigned merged;
            public:
	  enum State { StatePoll=0, StateReady=1, StateIdle=2, StateIdleUseful=3, StateLast=4 };
	  int64_t state_timers[ StateLast ];
    double state_timers_d[ StateLast ];
	  State prev_state;
	  int64_t scheduler_count;

    TaskingSchedulerStatistics() { active_task_log = new short[16]; }  // only for declarations that will be copy-assigned to
	  TaskingSchedulerStatistics( TaskingScheduler * scheduler )
                    : sched( scheduler ) 
#ifdef VTRACE_SAMPLED
		    , tasking_scheduler_grp_vt( VT_COUNT_GROUP_DEF( "Tasking scheduler" ) )
		    , active_tasks_out_ev_vt( VT_COUNT_DEF( "Active workers", "tasks", VT_COUNT_TYPE_UNSIGNED, tasking_scheduler_grp_vt ) )
		    , num_idle_out_ev_vt( VT_COUNT_DEF( "Idle workers", "tasks", VT_COUNT_TYPE_UNSIGNED, tasking_scheduler_grp_vt ) )
		    , readyQ_size_ev_vt( VT_COUNT_DEF( "ReadyQ size", "workers", VT_COUNT_TYPE_UNSIGNED, tasking_scheduler_grp_vt ) )
#endif
		    , merged(1)
	    , state_timers()
      , state_timers_d()
		  , prev_state( StateIdle )
	    , scheduler_count(0)
	    
	  {
	    for( int i = StatePoll; i < StateLast; ++i ) {
	      state_timers[ i ] = 0;
        state_timers_d[ i ] = 0.0f;
	    }
	    active_task_log = new short[1L<<20];
	    reset();
	  }
                ~TaskingSchedulerStatistics() {
                  // XXX: not copy-safe, so pointer can be invalid
                    /* delete[] active_task_log; */
                }

                void reset() {
                    merged = 1;
                    task_calls = 0;
                    task_log_index = 0;

                    max_active = 0;
                    avg_active = 0.0;
                    avg_ready = 1.0;

                    for (int i=StatePoll; i<StateLast; i++) state_timers[i] = 0;
                    scheduler_count = 0;

                }
                void print_active_task_log() {
#ifdef DEBUG
                    if (task_log_index == 0) return;

                    std::stringstream ss;
                    for (int64_t i=0; i<task_log_index; i++) ss << active_task_log[i] << " ";
                    LOG(INFO) << "Active tasks log: " << ss.str();
#endif
                }
                void dump() {
                  if (merged==1) { // state_timers_d invalid
                    std::cout << "TaskingSchedulerStatistics { "
                        << "max_active: " << max_active << ", "
                        << "avg_active: " << avg_active << ", " 
                        << "avg_ready: " << avg_ready
			      << ", scheduler_polling_thread_ticks: " << state_timers[ StatePoll ]
			      << ", scheduler_ready_thread_ticks: " << state_timers[ StateReady ]
			      << ", scheduler_idle_thread_ticks: " << state_timers[ StateIdle ]
			      << ", scheduler_idle_useful_thread_ticks: " << state_timers[ StateIdleUseful ]
			      << ", scheduler_count: " << scheduler_count
			      << " }" << std::endl;
                  } else {
                    CHECK( merged > 1 );
                    std::cout << "TaskingSchedulerStatistics { "
                        << "max_active: " << max_active << ", "
                        << "avg_active: " << avg_active << ", " 
                        << "avg_ready: " << avg_ready
			      << ", scheduler_polling_thread_ticks: " << state_timers_d[ StatePoll ]
			      << ", scheduler_ready_thread_ticks: " << state_timers_d[ StateReady ]
			      << ", scheduler_idle_thread_ticks: " << state_timers_d[ StateIdle ]
			      << ", scheduler_idle_useful_thread_ticks: " << state_timers_d[ StateIdleUseful ]
			      << ", scheduler_count: " << scheduler_count
			      << " }" << std::endl;
                  }
                }
                void sample();
	        void profiling_sample();
                void merge(const TaskingSchedulerStatistics * other); 

        };
       
       TaskingSchedulerStatistics stats;
  
       TaskingScheduler ( );
       void init ( Thread * master, TaskManager * taskman );

       Thread * get_current_thread() {
           return current_thread;
       }
       
       void assignTid( Thread * thr ) {
           thr->id = nextId++;
       }
        
      void createWorkers( uint64_t num );
      Thread* maybeSpawnCoroutines( );
      void onWorkerStart( );

       uint64_t active_task_count() {
         return num_active_tasks;
       }

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
  
       void merge_stats();
  
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
       friend void workerLoop ( Thread *, void * );
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
    StateTimer::enterState_scheduler();

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
    StateTimer::enterState_scheduler();

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
    StateTimer::enterState_scheduler();
    
    Thread * yieldedThr = current_thread;
    yieldedThr->co->running = 0; // XXX: hack; really want to know at a user Thread level that it isn't running
    yieldedThr->co->suspended = 1;
    Thread * next = nextCoroutine( );
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

/// Wake a suspended Thread by putting it on the run queue.
/// For now, waking a running Thread is a fatal error.
/// For now, waking a queued Thread is also a fatal error. 
/// For now, can only wake a Thread on your scheduler
inline void TaskingScheduler::thread_wake( Thread * next ) {
  CHECK( next->sched == this ) << "can only wake a Thread on your scheduler (next="<<(void*) next << " next->sched="<<(void*)next->sched <<" this="<<(void*)this;
  CHECK( next->next == NULL ) << "woken Thread should not be on any queue";
  CHECK( !thread_is_running( next ) ) << "woken Thread should not be running";
  
  next->co->suspended = 0;
    
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
    StateTimer::enterState_scheduler();
    
    next->co->suspended = 0;

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
    StateTimer::enterState_scheduler();
  
    next->co->suspended = 0;
  
    Thread * yieldedThr = current_thread;
    yieldedThr->co->suspended = 1;

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

