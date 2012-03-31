#ifndef _TASKING_SCHEDULER_HPP_
#define _TASKING_SCHEDULER_HPP_

#include "Scheduler.hpp"
#include <glog/logging.h>

class TaskManager;
struct task_worker_args;

class TaskingScheduler : public Scheduler {
    private:
        ThreadQueue readyQ;
        ThreadQueue periodicQ;
        ThreadQueue unassignedQ;

        thread * master;
        thread * current_thread; 
        threadid_t nextId;
        
        uint64_t num_idle;
        TaskManager * task_manager;
        uint64_t num_workers;
        
        thread * getWorker ();

        task_worker_args* work_args;

        // STUB: replace with real periodic threads
        int periodctr;
        thread * periodicDequeue() {
            if (periodctr++ % 100 == 0) {
                return periodicQ.dequeue();
            } else {
                return NULL;
            }
        }
        //////

        thread * nextCoroutine ( bool isBlocking=true ) {
            do {
                thread* result;

                // check for periodic tasks
                result = periodicDequeue();
                if (result != NULL) return result;

                // check ready tasks
                result = readyQ.dequeue();
                if (result != NULL) return result;

                // check for new workers
                result = getWorker();
                if (result != NULL) return result;

                // no coroutines can run, so handle
                DVLOG(5) << "scheduler: no coroutines can run";
                usleep(1);
            } while ( isBlocking || (!periodicQ.empty() || !unassignedQ.empty()) ); 
            // exit if all threads exited, including idle workers
            // TODO just as use mightBeWork as shortcut, also kill all idle unassigned workers on cbarrier_exit
            
            return NULL;
        }


    public:
       TaskingScheduler ( thread * master, TaskManager * taskman ); 

       thread * get_current_thread() {
           return current_thread;
       }
       
       void assignTid( thread * thr ) {
           thr->id = nextId++;
       }
        
      void createWorkers( uint64_t num );
      thread* maybeSpawnCoroutines( );

       void unassigned( thread * thr ) {
           unassignedQ.enqueue( thr );
       }

       void ready( thread * thr ) {
            readyQ.enqueue( thr );
       }

       void periodic( thread * thr ) {
           periodicQ.enqueue( thr );
       }

       /// run threads until all exit 
       void run ( ); 

       bool thread_yield( );
       void thread_suspend( );
       void thread_wake( thread * next );
       void thread_yield_wake( thread * next );
       void thread_suspend_wake( thread * next );
       bool thread_idle( uint64_t total_idle ); 
       bool thread_idle( );
       void thread_join( thread* wait_on );

       // Start running threads from <scheduler> until one dies.  Return that thread
       // (which can't be restarted--it's trash.)  If <result> non-NULL,
       // store the thread's exit value there.
       // If no threads to run, returns NULL.
       thread * thread_wait( void **result );


       void thread_on_exit( );
};  

struct task_worker_args {
    TaskManager *const tasks;
    TaskingScheduler *const scheduler;

    task_worker_args( TaskManager * task_manager, TaskingScheduler * sched )
        : tasks( task_manager )
        , scheduler( sched ) { }
};
       

/// Yield the CPU to the next thread on your scheduler.  Doesn't ever touch
/// the master thread.
inline bool TaskingScheduler::thread_yield( ) {
    CHECK( current_thread != master ) << "can't yield on a system thread";

    ready( current_thread ); 
    
    thread * yieldedThr = current_thread;

    thread * next = nextCoroutine( );
    bool gotRescheduled = (next == yieldedThr);
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
    
    return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}


/// Suspend the current thread. Thread is not placed on any queue.
inline void TaskingScheduler::thread_suspend( ) {
    CHECK( current_thread != master ) << "can't yield on a system thread";
    CHECK( thread_is_running(current_thread) ) << "may only suspend a running coroutine";
    
    thread* next = nextCoroutine( );

    thread* yieldedThr = current_thread;
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

/// Wake a suspended thread by putting it on the run queue.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
/// For now, can only wake a thread on your scheduler
inline void TaskingScheduler::thread_wake( thread * next ) {
  CHECK( next->sched == this ) << "can only wake a thread on your scheduler";
  CHECK( next->next == NULL ) << "woken thread should not be on any queue";
  CHECK( !thread_is_running( next ) ) << "woken thread should not be running";

  ready( next );
}

/// Yield the current thread and wake a suspended thread.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void TaskingScheduler::thread_yield_wake( thread * next ) {    
    CHECK( current_thread != master ) << "can't yield on a system thread";
    CHECK( next->sched == this ) << "can only wake a thread on your scheduler";
    CHECK( next->next == NULL ) << "woken thread should not be on any queue";
    CHECK( !thread_is_running( next ) ) << "woken thread should not be running";
  
    thread * yieldedThr = current_thread;
    ready( yieldedThr );
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

/// Suspend current thread and wake a suspended thread.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void TaskingScheduler::thread_suspend_wake( thread *next ) {
    CHECK( current_thread != master ) << "can't yield on a system thread";
    CHECK( next->next == NULL ) << "woken thread should not be on any queue";
    CHECK( !thread_is_running( next ) ) << "woken thread should not be running";
  
    thread * yieldedThr = current_thread;
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

    
/// Make the current thread idle.
/// Like suspend except the thread is not blocking on a
/// particular resource, just waiting to be woken.
inline bool TaskingScheduler::thread_idle( uint64_t total_idle ) {
    if (num_idle+1 == total_idle) {
        return false;
    } else {
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
  thread * exitedThr = current_thread;
  current_thread = master;

  thread_context_switch( exitedThr, master, (void *)exitedThr);
}




#endif

