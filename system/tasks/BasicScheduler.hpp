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
