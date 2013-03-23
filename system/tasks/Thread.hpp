// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef THREAD_HPP
#define THREAD_HPP

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <coro.h>
#include <boost/cstdint.hpp>
#include <iostream>

#ifdef VTRACE
#include <vt_user.h>
#endif

#include "StateTimer.hpp"
#include "PerformanceTools.hpp"

DECLARE_int64( stack_size );

class Scheduler;
typedef uint32_t threadid_t; 
struct Thread;

/// Size in bytes of the stack allocated for every Thread
//const size_t STACK_SIZE = 1L<<19;
//const size_t STACK_SIZE = 1L<<14;

/// A queue of threads
class ThreadQueue {
    private:
        Thread * head;
        Thread * tail;
        int len;
        
        std::ostream& dump( std::ostream& o ) const {
            return o << "[length:" << len
                     << "; head:" << (void*)head
                     << "; tail:" << (void*)tail << "]";
        }

    public:
        ThreadQueue ( ) 
            : head ( NULL )
            , tail ( NULL )
            , len ( 0 )
        { }

  inline void enqueue(Thread * t);
  inline Thread * dequeue();
  inline void prefetch();
        int length() { 
            return len;
        }

  inline bool empty() {
            return (head==NULL);
        }
        
        friend std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq );
};

#ifdef GRAPPA_TRACE
// keeps track of last id assigned
extern int thread_last_tau_taskid;
#endif

/// Grappa lightweight thread. 
/// Includes meta data and a coroutine (stack and stack pointer)
struct Thread {
  Thread * next; // for queues--if we're not in one, should be NULL
  coro co;
  void * data_prefetch;
  int done;

  // Scheduler responsible for this Thread. NULL means this is a system
  // Thread.
  Scheduler * sched;
  threadid_t id; 
  ThreadQueue joinqueue;
#ifdef GRAPPA_TRACE
  int state;
  int tau_taskid;
#endif

  /// Thread constructor. Thread belongs to Scheduler.
  /// To actually spawn the Thread * thread_spawn(Thread*,Scheduler*,thread_func,void*)
  /// must be used.
  Thread(Scheduler * sched) 
    : next( NULL )
    , co()
    , data_prefetch( NULL )
    , done( 0 )
    , sched( sched )
    , id(-1)
    , joinqueue( ) 
#ifdef GRAPPA_TRACE
    , state ( 0 ) 
#endif
    {
        // NOTE: id, co still need to be initialized later
    }

  /// prefetch the Thread execution state
  inline void prefetch() {
    __builtin_prefetch( next, 1, 3 );                           // try to keep next thread in cache
    __builtin_prefetch( co.stack, 1, 3 );                           // try to keep stack in cache
    //if( data_prefetch ) __builtin_prefetch( data_prefetch, 0, 0 );   // for low-locality data
  }
} __attribute__((aligned(64)));
        
/// Remove a Thread from the queue and return it
inline Thread * ThreadQueue::dequeue() {
    Thread * result = head;
    if (result != NULL) {
        head = result->next;
        result->next = NULL;
        len--;
    } else {
        tail = NULL;
    }
    return result;
}

/// Add a Thread to the queue
inline void ThreadQueue::enqueue( Thread * t) {
    if (head==NULL) {
        head = t;
    } else {
        tail->next = t;
    }
    tail = t;
    t->next = NULL;
    len++;
}


/// Prefetch some Thread close to the front of the queue
inline void ThreadQueue::prefetch() {
    Thread * result = head;
    if( result ) {
      result->prefetch();
    }
}

typedef void (*thread_func)(Thread *, void *arg);


Thread * thread_spawn ( Thread * me, Scheduler * sched, thread_func f, void * arg);

Thread * thread_init();

/// Perform a context switch to another Thread
/// @param running the current Thread
/// @param next the Thread to switch to
/// @param val pass a value to next
inline void* thread_context_switch( Thread * running, Thread * next, void * val ) {
    // This timer ensures we are able to calculate exclusive time for the previous thing in this thread's callstack,
    // so that we don't count time in another thread
    GRAPPA_THREAD_FUNCTION_PROFILE( GRAPPA_SUSPEND_GROUP, running );  
#ifdef VTRACE_FULL
  VT_TRACER("context switch");
#endif
    void* res = coro_invoke( &(running->co), &(next->co), val );
    StateTimer::enterState_thread();
    return res; 
}

/// Return true if the thread is in the running state
inline int thread_is_running( Thread * thr ) {
  return thr->co.running;
}

void thread_exit( Thread * me, void * retval );

void destroy_thread( Thread *thr );


#endif // THREAD_HPP
