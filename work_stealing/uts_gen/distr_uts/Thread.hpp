#ifndef THREAD_HPP
#define THREAD_HPP

#include "coro.h"

typedef uint32_t threadid_t; 

class Scheduler;
class ThreadQueue;

typedef struct thread {
  coro *co;
  // Scheduler responsible for this thread. NULL means this is a system
  // thread.
  Scheduler * sched;
  thread * next; // for queues--if we're not in one, should be NULL
  threadid_t id; 
  ThreadQueue joinqueue;
  int done;
} thread;

extern thread * current_thread;

thread * get_current_thread();

typedef void (*thread_func)(thread *, void *arg);


class ThreadQueue {
    private:
        thread* head;
        thread* tail;

    public:
        ThreadQueue ( ) 
            : head ( NULL ),
            : tail ( NULL ) { }

        thread* dequeue() {
            thread* result = head;
            if (result != NULL) {
                head = result->next;
                result->next = NULL;
            } else {
                tail = NULL;
            }
            return result;
        }

        void enqueue(thread* t) {
            if (head==NULL) {
                head = t;
            } else {
                tail->next = t;
            }
            tail = thread;
            t->next = NULL;
        }
};


class Scheduler () {
    private:
        ThreadQueue readyQ;
        ThreadQueue periodicQ;
        ThreadQueue unassignedQ;

        thread * master;
        threadid_t nextId;
        uint64_t num_idle;

    public:
       Scheduler ( thread * master ) 
        : readyQ ( )
        , periodicQ ( )
        , unassignedQ ( )
        , master ( master )
        , nextID ( 1 )
        , num_idle ( 0 ) { }

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

       /// run threads until all exit 
       void run ( ); 

       thread * nextCoroutine () {

           while ( true ) {
               thread* result;

               // check for periodic tasks
               result = periodicQ.dequeue();
               if (result != NULL_THREAD) return result;

               // check ready tasks
               result = readyQ.dequeue();
               if (result != NULL_THREAD) return result;

               // check for new workers
               result = getWorker();
               if (result != NULL_THREAD) return result;

               // no coroutines can run, so handle
               DVLOG(5) << "scheduler: no coroutines can run";
               usleep(1);
           }
       }
};


thread * thread_spawn ( thread * me, Scheduler * sched, thread_func f, void * arg);


// Turns the current (system) thread into a green thread.
thread * thread_init();

/// Yield the CPU to the next thread on your scheduler.  Doesn't ever touch
/// the master thread.
inline int thread_yield(thread *me) {
    scheduler *sched = me->sched;
    // can't yield on a system thread
    assert(sched != NULL);

    sched->ready( me ); 

    thread * next = sched->nextCoroutine( );

    current_thread = next;
    coro_invoke(me->co, next->co, NULL);
    
    return next == me; // 0=another ran; 1=me got rescheduled immediately
}

/// Suspend the current thread. Thread is not placed on any queue.
static inline int thread_suspend(thread *me) {
    scheduler *sched = me->sched;
    // can't yield on a system thread
    assert(sched != NULL);

    // may only suspend a running coroutine
    assert( me->co->running == 1 );
    
    thread *next = sched->nextCoroutine( );

    current_thread = next;
    coro_invoke(me->co, next->co, NULL);
    return 0;
}

/// Wake a suspended thread by putting it on the run queue.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void thread_wake(thread *next) {
  scheduler *sched = next->sched;
  // can't yield on a system thread
  assert(sched != NULL);
  assert( next->next == NULL && next->co->running == 0 );

  sched->ready( next );
}

/// Yield the current thread and wake a suspended thread.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void thread_yield_wake(thread *me, thread *next) {
  scheduler *sched = me->sched;
  // can't yield on a system thread
  assert(sched != NULL);
  if (sched->ready == NULL) {
      return;
  } else {
    sched->ready( me );
    current_thread = next;
    // the thread to wake should not be on a queue and should not be running
    assert( next->next == NULL && next->co->running == 0 );
    coro_invoke(me->co, next->co, NULL);
  }
}

/// Suspend current thread and wake a suspended thread.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void thread_suspend_wake(thread *me, thread *next) {
  scheduler *sched = me->sched;
  // can't yield on a system thread
  assert(sched != NULL);
  
  current_thread = next;
  
  // the thread to wake should not be on a queue and should not be running
  assert( next->next == NULL && next->co->running == 0 );
  coro_invoke(me->co, next->co, NULL);
}

    
/// Make the current thread idle.
/// Like suspend except the thread is not blocking
/// on a particular resource, just waiting to be woken.
inline int thread_idle(thread* me, uint64_t total_idle) {
    scheduler* sched = me->sched;
    if (sched->num_idle+1 == total_idle) {
        return 0;
    } else {
        sched->num_idle++;
        
        sched->unassigned( me );
        
        thread_suspend(me);
        sched->num_idle--;
        return 1;
    }
}



// Die and return to the master thread.
void thread_exit(thread *me, void *retval);

// Delete a thread.
void destroy_thread(thread *thr);

// Delete a scheduler -- only call from master.
void destroy_scheduler(scheduler *sched);

// Start running threads from <scheduler> until one dies.  Return that thread
// (which can't be restarted--it's trash.)  If <result> non-NULL,
// store the thread's exit value there.
// If no threads to run, returns NULL.
thread *thread_wait(scheduler *sched, void **result);




inline void prefetch_and_switch(thread *me, void *addr, int ro_or_rw) {
  __builtin_prefetch(addr, 0, 0);
  thread_yield(me);
}

void thread_join(thread* me, thread* wait_on);



#endif

