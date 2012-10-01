// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef BASIC_SCHEDULER_HPP
#define BASIC_SCHEDULER_HPP

#include "Scheduler.hpp"
#include <Timestamp.hpp>
#include <glog/logging.h>


DECLARE_int64( periodic_poll_ticks );

/// A basic Thread Scheduler with just a readyQ and periodicQ
class BasicScheduler : public Scheduler {
    private:
        ThreadQueue readyQ;
        ThreadQueue periodicQ;

        Thread * master;
        Thread * current_thread; 
        threadid_t nextId;
        
        // STUB: replace with real periodic threads
        Grappa_Timestamp previous_periodic_ts;
        int periodctr;
        Thread * periodicDequeue() {
	    // tick the timestap counter
	    Grappa_tick();
	    Grappa_Timestamp current_ts = Grappa_get_timestamp();

	    if( current_ts - previous_periodic_ts > FLAGS_periodic_poll_ticks ) {
                return periodicQ.dequeue();
            } else {
                return NULL;
            }
        }
        //////

        Thread * nextCoroutine ( bool isBlocking=true ) {
            do {
                VLOG(5) << "scheduler: next coro";
                Thread * result;

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
       BasicScheduler ( Thread * master ) 
        : readyQ ( )
        , periodicQ ( )
        , master ( master )
        , current_thread ( master )
        , nextId ( 1 ) {
            
            periodctr = 0;/*XXX*/
        }
       
       Thread * get_current_thread() {
           return current_thread;
       }
       
       void assignTid( Thread * thr ) {
           thr->id = nextId++;
       }
        
       void ready( Thread * thr ) {
            readyQ.enqueue( thr );
       }

       void periodic( Thread * thr ) {
           periodicQ.enqueue( thr );
       }

       /// run threads until all exit 
       void run ( ); 

       bool thread_yield( );
       void thread_suspend( );
       void thread_wake( Thread * next );
       void thread_yield_wake( Thread * next );
       void thread_suspend_wake( Thread * next );
       void thread_join( Thread* wait_on );

       // Start running threads from <scheduler> until one dies.  Return that Thread
       // (which can't be restarted--it's trash.)  If <result> non-NULL,
       // store the Thread's exit value there.
       // If no threads to run, returns NULL.
       Thread * thread_wait( void **result );


       void thread_on_exit( );
};  


/// Yield the CPU to the next Thread on your scheduler.  Doesn't ever touch
/// the master Thread.
inline bool BasicScheduler::thread_yield( ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";

    ready( current_thread ); 
    
    Thread * yieldedThr = current_thread;

    Thread * next = nextCoroutine( );
    bool gotRescheduled = (next == yieldedThr);
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
    
    return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}


/// Suspend the current Thread. Thread is not placed on any queue.
inline void BasicScheduler::thread_suspend( ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";
    CHECK( thread_is_running(current_thread) ) << "may only suspend a running coroutine";
    
    Thread * yieldedThr = current_thread;
    
    Thread * next = nextCoroutine( );
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

/// Wake a suspended Thread by putting it on the run queue.
/// For now, waking a running Thread is a fatal error.
/// For now, waking a queued Thread is also a fatal error. 
/// For now, can only wake a Thread on your scheduler
inline void BasicScheduler::thread_wake( Thread * next ) {
  CHECK( next->sched == this ) << "can only wake a Thread on your scheduler";
  CHECK( next->next == NULL ) << "woken Thread should not be on any queue";
  CHECK( !thread_is_running( next ) ) << "woken Thread should not be running";

  ready( next );
}

/// Yield the current Thread and wake a suspended thread.
/// For now, waking a running Thread is a fatal error.
/// For now, waking a queued Thread is also a fatal error. 
inline void BasicScheduler::thread_yield_wake( Thread * next ) {    
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
inline void BasicScheduler::thread_suspend_wake( Thread *next ) {
    CHECK( current_thread != master ) << "can't yield on a system Thread";
    CHECK( next->next == NULL ) << "woken Thread should not be on any queue";
    CHECK( !thread_is_running( next ) ) << "woken Thread should not be running";
  
    Thread * yieldedThr = current_thread;
    
    current_thread = next;
    thread_context_switch( yieldedThr, next, NULL);
}

    
inline void BasicScheduler::thread_on_exit( ) {
  Thread * exitedThr = current_thread;
  current_thread = master;

  thread_context_switch( exitedThr, master, (void *)exitedThr);
}




#endif // BASIC_SCHEDULER_HPP

