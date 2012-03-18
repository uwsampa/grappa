#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "Thread.hpp"
#include "Task.hpp"


class Scheduler {
    private:
        ThreadQueue readyQ;
        ThreadQueue periodicQ;
        ThreadQueue unassignedQ;

        thread * master;
        threadid_t nextId;
        uint64_t num_idle;
        thread * current_thread;
        TaskManager * task_manager;
        
        thread * getWorker ();

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
            } while ( isBlocking || (periodicQ.empty() && unassignedQ.empty()) ); 
            // exit if all threads exited, including idle workers
            // TODO just as use mightBeWork as shortcut, also kill all idle unassigned workers on cbarrier_exit
            
            return NULL;
        }


    public:
       Scheduler ( thread * master, TaskManager * taskman ) 
        : readyQ ( )
        , periodicQ ( )
        , unassignedQ ( )
        , master ( master )
        , current_thread ( master )
        , nextId ( 1 )
        , num_idle ( 0 )
        , task_manager ( taskman ) { periodctr = 0;/*XXX*/}

       void assignTid( thread * thr ) {
           thr->id = nextId++;
       }

       void unassigned( thread * thr ) {
           unassignedQ.enqueue( thr );
       }

       void ready( thread * thr ) {
            readyQ.enqueue( thr );
       }

       void periodic( thread * thr ) {
           periodicQ.enqueue( thr );
       }

       thread * get_current_thread( ) {
           return current_thread;
       }

       /// run threads until all exit 
       void run ( ); 

       bool thread_yield( );
       void thread_suspend( );
       void thread_wake( thread * next );
       void thread_yield_wake( thread * next );
       void thread_suspend_wake( thread * next );
       bool thread_idle( uint64_t total_idle ); 
       void thread_join( thread* wait_on );

       // Start running threads from <scheduler> until one dies.  Return that thread
       // (which can't be restarted--it's trash.)  If <result> non-NULL,
       // store the thread's exit value there.
       // If no threads to run, returns NULL.
       thread * thread_wait( void **result );


       void thread_on_exit( );
};  


/// Yield the CPU to the next thread on your scheduler.  Doesn't ever touch
/// the master thread.
inline bool Scheduler::thread_yield( ) {
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
inline void Scheduler::thread_suspend( ) {
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
inline void Scheduler::thread_wake( thread * next ) {
  CHECK( next->sched == this ) << "can only wake a thread on your scheduler";
  CHECK( next->next == NULL ) << "woken thread should not be on any queue";
  CHECK( !thread_is_running( next ) ) << "woken thread should not be running";

  ready( next );
}

/// Yield the current thread and wake a suspended thread.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void Scheduler::thread_yield_wake( thread * next ) {    
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
inline void Scheduler::thread_suspend_wake( thread *next ) {
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
inline bool Scheduler::thread_idle( uint64_t total_idle ) {
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

inline void Scheduler::thread_on_exit( ) {
  thread * exitedThr = current_thread;
  current_thread = master;

  thread_context_switch( exitedThr, master, (void *)exitedThr);
}




thread* Scheduler::getWorker () {
    if (task_manager->available()) {
        // check the pool of unassigned coroutines
        thread* result = unassignedQ.dequeue();
        if (result != NULL) return result;

        // possibly spawn more coroutines
        result = task_manager->maybeSpawnCoroutines();
        return result;
    } else {
        return NULL;
    }
}

#endif

