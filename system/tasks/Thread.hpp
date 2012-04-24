#ifndef THREAD_HPP
#define THREAD_HPP

#include <coro.h>
#include <boost/cstdint.hpp>
#include <iostream>

class Scheduler;
typedef uint32_t threadid_t; 
struct Thread;

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
            , len ( 0 ) { }

        void enqueue(Thread * t);
        Thread * dequeue();
        
        int length() { 
            return len;
        }

        bool empty() {
            return (head==NULL);
        }
        
        friend std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq );
};

struct Thread {
  coro *co;
  // Scheduler responsible for this Thread. NULL means this is a system
  // Thread.
  Scheduler * sched;
  Thread * next; // for queues--if we're not in one, should be NULL
  threadid_t id; 
  ThreadQueue joinqueue;
  int done;

  Thread(Scheduler * sched) 
    : sched( sched )
    , next( NULL )
    , done( 0 )
    , joinqueue( ) { 
        // NOTE: id, co still need to be initialized later
    }
};
        
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


typedef void (*thread_func)(Thread *, void *arg);


Thread * thread_spawn ( Thread * me, Scheduler * sched, thread_func f, void * arg);

// Turns the current (system) thread into a green thread.
Thread * thread_init();

inline void* thread_context_switch( Thread * running, Thread * next, void * val ) {
    return coro_invoke( running->co, next->co, val );
}

inline int thread_is_running( Thread * thr ) {
    return thr->co->running;
}

// Die and return to the master thread.
void thread_exit( Thread * me, void * retval );

// Delete a thread.
void destroy_thread( Thread *thr );


#endif
