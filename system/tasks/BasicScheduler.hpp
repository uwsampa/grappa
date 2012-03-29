#ifndef _BASIC_SCHEDULER_HPP_
#define _BASIC_SCHEDULER_HPP_

#include "Scheduler.hpp"
#include <glog/logging.h>

/// A basic scheduler with just a readyQ and periodicQ

class BasicScheduler : public Scheduler {
    private:
        ThreadQueue readyQ;
        ThreadQueue periodicQ;

        thread * master;
        thread * current_thread; 
        threadid_t nextId;
        
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
                VLOG(5) << "scheduler: next coro";
                thread* result;

                // check for periodic tasks
                result = periodicDequeue();
                if (result != NULL) return result;

                // check ready tasks
                result = readyQ.dequeue();
                if (result != NULL) return result;

                // no coroutines can run, so handle
                VLOG(5) << "scheduler: no coroutines can run";
                usleep(1);
            } while ( isBlocking || !periodicQ.empty() ); 
            
            VLOG(5) << "scheduler: returning coro NULL";
            return NULL;
        }


    public:
       BasicScheduler ( thread * master ) 
        : readyQ ( )
        , periodicQ ( )
        , master ( master )
        , current_thread ( master )
        , nextId ( 1 ) {
            
            periodctr = 0;/*XXX*/
        }
       
       thread * get_current_thread() {
           return current_thread;
       }
       
       void assignTid( thread * thr ) {
           thr->id = nextId++;
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
inline bool BasicScheduler::thread_yield( ) {
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
inline void BasicScheduler::thread_suspend( ) {
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
inline void BasicScheduler::thread_wake( thread * next ) {
  CHECK( next->sched == this ) << "can only wake a thread on your scheduler";
  CHECK( next->next == NULL ) << "woken thread should not be on any queue";
  CHECK( !thread_is_running( next ) ) << "woken thread should not be running";

  ready( next );
}

/// Yield the current thread and wake a suspended thread.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void BasicScheduler::thread_yield_wake( thread * next ) {    
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
inline void BasicScheduler::thread_suspend_wake( thread *next ) {
    CHECK( current_thread != master ) << "can't yield on a system thread";
    CHECK( next->next == NULL ) << "woken thread should not be on any queue";
    CHECK( !thread_is_running( next ) ) << "woken thread should not be running";
  
    thread * yieldedThr = current_thread;
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

    
inline void BasicScheduler::thread_on_exit( ) {
  thread * exitedThr = current_thread;
  current_thread = master;

  thread_context_switch( exitedThr, master, (void *)exitedThr);
}




#endif

