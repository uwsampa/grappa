#ifndef THREAD_H
#include "coro.h"

struct scheduler;

typedef struct thread {
  coro *coro;
  // Scheduler responsible for this thread. NULL means this is a system
  // thread.
  struct scheduler *sched;
  struct thread *next; // for queues--if we're not in one, should be NULL
} thread;

typedef struct scheduler {
  thread *ready; // ready queue
  thread *tail; // tail of queue -- undefined if ready == NULL
  thread *master; // the "real" thread
} scheduler;

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
void thread_yield(thread *me);

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
#endif
