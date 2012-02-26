
#ifdef __cplusplus
extern "C" {
#endif


#ifndef THREAD_H
#define THREAD_H

#include <assert.h>
#include <stdint.h>

#include "coro.h"

struct scheduler;

typedef uint32_t threadid_t; 

typedef struct thread {
  coro *co;
  // Scheduler responsible for this thread. NULL means this is a system
  // thread.
  struct scheduler *sched;
  struct thread *next; // for queues--if we're not in one, should be NULL
  threadid_t id; 
  unsigned long wake_time;
  struct thread* join_head;
  struct thread* join_tail;
  int done;
} thread;

extern thread * current_thread;

thread * get_current_thread();

typedef struct scheduler {
  thread * ready; // ready queue
  thread * tail; // tail of queue -- undefined if ready == NULL
  thread * idle_head;
  thread * idle_tail;
  thread * master; // the "real" thread
  threadid_t nextId;
  uint64_t num_idle;
  int idleReady;
} scheduler;

inline void scheduler_enqueue(scheduler *sched, thread *thr) {
  if (sched->ready == NULL) {
    sched->ready = thr;
    assert(thr!=NULL);
  } else {
    sched->tail->next = thr;
  }
  sched->tail = thr;
}

inline thread *scheduler_dequeue(scheduler *sched) {
  thread *result = sched->ready;
  if (result != NULL) {
    sched->ready = result->next;
    result->next = NULL;
  }
  return result;
}


void scheduler_assignTid(scheduler *sched, thread* thr);

typedef void (*thread_func)(thread *, void *arg);

// Turns the current (system) thread into a green thread.
thread *thread_init();

scheduler *create_scheduler(thread *master);

// Will create a thread that calls <f>(<arg>) and place it on the
// ready queue.  Safe to call from <master> or any child of <scheduler>.
thread *thread_spawn(thread *me, scheduler *sched,
                     thread_func f, void *arg);

inline void unassigned_enqueue(scheduler* sched, thread* thr) {
   if (sched->idle_head == NULL) {
       sched->idle_head = thr;
   } else {
       sched->idle_tail->next = thr;
   }
   sched->idle_tail = thr;
   thr->next = NULL;
}

inline thread* unassigned_dequeue(scheduler* sched) {
    thread* result = sched->idle_head;
    if (result != NULL) {
        sched->idle_head = result->next;
        result->next = NULL;
    } else {
        sched->idle_tail = NULL;
    }
    return result;
}

// Yield the CPU to the next thread on your scheduler.  Doesn't ever touch
// the master thread.
inline void thread_yield(thread *me) {
    scheduler *sched = me->sched;
    // can't yield on a system thread
    assert(sched != NULL);
    thread *next;
    if (sched->idleReady && sched->idle_head!=NULL) { //first to prevent starvation by system threads, but might be better to have separate queue for system threads
        scheduler_enqueue(sched, me);
        next = unassigned_dequeue(sched);
    } else if (sched->ready == NULL) { 
        return;
    } else {
        scheduler_enqueue(sched, me);
        next = scheduler_dequeue(me->sched);
    }
    current_thread = next;
    coro_invoke(me->co, next->co, NULL);
}

/// Suspend the current thread. Thread is not placed on any queue.
static inline void thread_suspend(thread *me) {
    scheduler *sched = me->sched;
    // can't yield on a system thread
    assert(sched != NULL);
    thread *next;
    if (sched->idleReady && sched->idle_head!=NULL) {
        next = unassigned_dequeue(sched);
    } else if (sched->ready == NULL) {
        // technically should never be allowed to happen
        assert(0); 
        return;
    } else {
        next = scheduler_dequeue(me->sched);
    }

    assert( me->co->running == 1 );
    current_thread = next;
    coro_invoke(me->co, next->co, NULL);
}

/// Wake a suspended thread by putting it on the run queue.
/// For now, waking a running thread is a fatal error.
/// For now, waking a queued thread is also a fatal error. 
inline void thread_wake(thread *next) {
  scheduler *sched = next->sched;
  // can't yield on a system thread
  assert(sched != NULL);
  assert( next->next == NULL && next->co->running == 0 );
  scheduler_enqueue(sched, next);
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
    scheduler_enqueue(sched, me);
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
  if (sched->ready == NULL) {
      return;
  } else {
     current_thread = next;
    // the thread to wake should not be on a queue and should not be running
    assert( next->next == NULL && next->co->running == 0 );
    coro_invoke(me->co, next->co, NULL);
  }
}

/// Make all threads in the unassigned pool eligible/ineligible for running.
inline void thread_idlesReady(thread* me, int ready) {
    scheduler *sched = me->sched;

    // take all threads on idle queue (if there are any) and put in pool of unassigned threads
    sched->idleReady = ready;
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
        unassigned_enqueue(sched, me);
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

// Helper function--runs all threads until nothing left
void run_all(scheduler *sched);

typedef struct thread_barrier {
  scheduler *sched;
  int size; // n needed to pass
  int blocked; // n blocked
  thread *threads; // the threads
} thread_barrier;

inline void prefetch_and_switch(thread *me, void *addr, int ro_or_rw) {
  __builtin_prefetch(addr, 0, 0);
  thread_yield(me);
}


// block on barrier until barrier->size threads there.
inline void thread_block(thread *me, thread_barrier *barrier) {
  me->next = barrier->threads;
  barrier->threads = me;
  if (++barrier->blocked == barrier->size) {
    barrier->blocked = 0;
    while (barrier->threads != NULL) {
      scheduler_enqueue(barrier->sched, barrier->threads);
      barrier->threads = barrier->threads->next;
    }
  } else {
    // assumption: programmer isn't stupid, won't leave us with nothing to run.
    // if he does, we're deadlocked anyway.
  }
  thread *next = scheduler_dequeue(barrier->sched);
  current_thread = next;
  coro_invoke(me->co, next->co, NULL);
}


void thread_join(thread* me, thread* wait_on);


#define rdtscll(val) do { \
    unsigned int __a,__d; \
    asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
    (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)

#define FIFO_WAIT 1
// Yield the CPU and don't wake until AT LEAST 'ticks' from now
inline void thread_yield_alarm(thread *me, unsigned long ticks) {
  scheduler *sched = me->sched;
  // can't yield on a system thread
  assert(sched != NULL);

  unsigned long current;
  rdtscll(current);
  me->wake_time = current + ticks;

  scheduler_enqueue(sched, me);

  // spin through queue until a thread has passed its wake_time
  // spin because there is no other work to do (although this h/w thread could go to sleep but we don't care about that)
  #if FIFO_WAIT
  int waked = 0;
  thread* next = scheduler_dequeue(sched);
  while (!waked) {
      rdtscll(current);
      if (current >= next->wake_time) {
        waked = 1;
      }
  }
  #else
  thread *next = NULL;  
  while (next == NULL) {  // basic impl of min sleep time that is unfair in ordering
    thread *potential = scheduler_dequeue(sched);
    
    rdtscll(current);
    if (current >= potential->wake_time) {
        next = potential;
    } else {
        scheduler_enqueue(sched, potential);
    }
  }
  #endif
  current_thread = next;
  coro_invoke(me->co, next->co, NULL); 
}


#endif

#ifdef __cplusplus
}
#endif

