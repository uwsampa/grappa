////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////
#pragma once

#include "Scheduler.hpp"
#include "ThreadQueue.hpp"
#include <Timestamp.hpp>
#include <glog/logging.h>


DECLARE_int64( periodic_poll_ticks );

namespace Grappa {

/// A basic Worker Scheduler with just a readyQ and periodicQ
class BasicScheduler : public Scheduler {
    private:
        ThreadQueue readyQ;
        ThreadQueue periodicQ;

        Worker * master;
        Worker * current_thread; 
        threadid_t nextId;
        
        // STUB: replace with real periodic threads
        Grappa::Timestamp previous_periodic_ts;
        int periodctr;
        Worker * periodicDequeue() {
	    // tick the timestap counter
	    Grappa::tick();
	    Grappa::Timestamp current_ts = Grappa::timestamp();

	    if( current_ts - previous_periodic_ts > FLAGS_periodic_poll_ticks ) {
                return periodicQ.dequeue();
            } else {
                return NULL;
            }
        }
        //////

        Worker * nextCoroutine ( bool isBlocking=true ) {
            do {
                VLOG(5) << "scheduler: next coro";
                Worker * result;

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
       BasicScheduler ( Worker * master ) 
        : readyQ ( )
        , periodicQ ( )
        , master ( master )
        , current_thread ( master )
        , nextId ( 1 ) {
            
            periodctr = 0;/*XXX*/
        }
       
       Worker * get_current_thread() {
           return current_thread;
       }
       
       void assignTid( Worker * thr ) {
           thr->id = nextId++;
       }
        
       void ready( Worker * thr ) {
            readyQ.enqueue( thr );
       }

       void periodic( Worker * thr ) {
           periodicQ.enqueue( thr );
       }

       /// run threads until all exit 
       void run ( ); 

       bool thread_yield( );
       void thread_suspend( );
       void thread_wake( Worker * next );
       void thread_yield_wake( Worker * next );
       void thread_suspend_wake( Worker * next );
       void thread_join( Worker* wait_on );

       // Start running threads from <scheduler> until one dies.  Return that Worker
       // (which can't be restarted--it's trash.)  If <result> non-NULL,
       // store the Worker's exit value there.
       // If no threads to run, returns NULL.
       Worker * thread_wait( void **result );


       void thread_on_exit( );
};  


/// Yield the CPU to the next Worker on your scheduler.  Doesn't ever touch
/// the master Worker.
inline bool BasicScheduler::thread_yield( ) {
    CHECK( current_thread != master ) << "can't yield on a system Worker";

    ready( current_thread ); 
    
    Worker * yieldedThr = current_thread;

    Worker * next = nextCoroutine( );
    bool gotRescheduled = (next == yieldedThr);
    
    current_thread = next;
    impl::thread_context_switch( yieldedThr, next, NULL);
    
    return gotRescheduled; // 0=another ran; 1=me got rescheduled immediately
}


/// Suspend the current Worker. Worker is not placed on any queue.
inline void BasicScheduler::thread_suspend( ) {
    CHECK( current_thread != master ) << "can't yield on a system Worker";
    CHECK( current_thread->running ) << "may only suspend a running coroutine";
    
    Worker * yieldedThr = current_thread;
    
    Worker * next = nextCoroutine( );
    
    current_thread = next;
    impl::thread_context_switch( yieldedThr, next, NULL);
}

/// Wake a suspended Worker by putting it on the run queue.
/// For now, waking a running Worker is a fatal error.
/// For now, waking a queued Worker is also a fatal error. 
/// For now, can only wake a Worker on your scheduler
inline void BasicScheduler::thread_wake( Worker * next ) {
  CHECK( next->sched == this ) << "can only wake a Worker on your scheduler";
  CHECK( next->next == NULL ) << "woken Worker should not be on any queue";
  CHECK( !next->running ) << "woken Worker should not be running";

  ready( next );
}

/// Yield the current Worker and wake a suspended thread.
/// For now, waking a running Worker is a fatal error.
/// For now, waking a queued Worker is also a fatal error. 
inline void BasicScheduler::thread_yield_wake( Worker * next ) {    
    CHECK( current_thread != master ) << "can't yield on a system Worker";
    CHECK( next->sched == this ) << "can only wake a Worker on your scheduler";
    CHECK( next->next == NULL ) << "woken Worker should not be on any queue";
    CHECK( !next->running ) << "woken Worker should not be running";
  
    Worker * yieldedThr = current_thread;
    ready( yieldedThr );
    
    current_thread = next;
    impl::thread_context_switch( yieldedThr, next, NULL);
}

/// Suspend current Worker and wake a suspended thread.
/// For now, waking a running Worker is a fatal error.
/// For now, waking a queued Worker is also a fatal error. 
inline void BasicScheduler::thread_suspend_wake( Worker *next ) {
    CHECK( current_thread != master ) << "can't yield on a system Worker";
    CHECK( next->next == NULL ) << "woken Worker should not be on any queue";
    CHECK( !next->running ) << "woken Worker should not be running";
  
    Worker * yieldedThr = current_thread;
    
    current_thread = next;
    impl::thread_context_switch( yieldedThr, next, NULL);
}

    
inline void BasicScheduler::thread_on_exit( ) {
  Worker * exitedThr = current_thread;
  current_thread = master;

  impl::thread_context_switch( exitedThr, master, (void *)exitedThr);
}


} // namespace Grappa
