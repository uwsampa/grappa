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
} thread;

typedef struct scheduler {
  thread *ready; // ready queue
  thread *tail; // tail of queue -- undefined if ready == NULL
  thread *wait;
  thread *wait_tail;
  thread *master; // the "real" thread
  threadid_t nextId;
} scheduler;

inline void scheduler_enqueue(scheduler *sched, thread *thr) {
  if (sched->ready == NULL) {
    sched->ready = thr;
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

inline void scheduler_wait_enqueue(scheduler *sched, thread *thr) {
  if (sched->wait == NULL) {
    sched->wait = thr;
    sched->wait_tail = thr;
  } else {
    sched->wait_tail->next = thr;
    sched->wait_tail = thr;
  }
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

// Yield the CPU to the next thread on your scheduler.  Doesn't ever touch
// the master thread.
inline void thread_yield(thread *me) {
  scheduler *sched = me->sched;
  // can't yield on a system thread
  assert(sched != NULL);
  if (sched->ready == NULL) {
      return;
  } else {
    scheduler_enqueue(sched, me);
    thread *next = scheduler_dequeue(sched);
    coro_invoke(me->co, next->co, NULL);
  }
}

inline int thread_yield_wait(thread* me) {
    scheduler *sched = me->sched;

    // can't yield on a system thread
    assert(sched != NULL);

    // if this was the last thread then return so the
    // user code can handle (e.g. okay to go to pthread barrier)
    if (sched->ready == NULL) {
        return 0;
    }

    scheduler_wait_enqueue(sched, me);

    thread *next = scheduler_dequeue(sched);
    coro_invoke(me->co, next->co, NULL);
    
    return 1;
}

inline void threads_wakeN(thread* me, int n) {
    scheduler *sched = me->sched;
    thread* waitHead = sched->wait;
    if (n>0 && waitHead!=NULL) {
        sched->tail->next = waitHead;
        thread* ptr = waitHead;
        int i = 1;
        while (i < n && ptr->next!=NULL) {
            ptr = ptr->next;
            i++;
        }   
        sched->wait = ptr->next;
        sched->tail = ptr;
    }
}

// assumes right now there is a single resource for threads in this scheduler to wait on
inline void threads_wakeAll(thread* me) {
    scheduler *sched = me->sched;

    // take all threads on wait queue (if there are any) and put in run queue
    thread* waitHead = sched->wait;
    if (waitHead!=NULL) {
        thread* waitTail = sched->wait_tail;
        sched->wait = NULL;
        sched->wait_tail = NULL;

        if (sched->ready == NULL) {  
            sched->ready = waitHead;
        } else {
            sched->tail->next = waitHead;
        }

        sched->tail = waitTail;
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
  __builtin_prefetch(addr, ro_or_rw, 0);
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
  coro_invoke(me->co, next->co, NULL);
}




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

  coro_invoke(me->co, next->co, NULL); 
}


#endif

#ifdef __cplusplus
}
#endif
