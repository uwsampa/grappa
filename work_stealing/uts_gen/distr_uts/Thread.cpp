#include "Thread.hpp"

#include <stdlib.h>


thread * current_thread;

thread * get_current_thread() {
  return current_thread;
}

thread *thread_init() {
  coro *me = coro_init();
  thread *master = (thread*)malloc(sizeof(thread));
  assert(master != NULL);
  master->co = me;
  master->sched = NULL;
  master->next = NULL;
  master->id = 0; // master always id 0
  master->done = 0;
  current_thread = master;
  return master;
}

static void tramp(struct coro *me, void *arg) {
  // Pass control back and forth a few times to get the info we need.
  coro *master = (coro *)arg;
  thread *my_thr = (thread *)coro_invoke(me, master, NULL);
  thread_func f = (thread_func)coro_invoke(me, master, NULL);
  void *f_arg = coro_invoke(me, master, NULL);
  // Next time we're invoked, it'll be for real.
  coro_invoke(me, master, NULL);

  f(my_thr, f_arg);

  // We shouldn't return, but if we do, kill the thread.
  thread_exit(my_thr, NULL);
}

/// Spawn a new thread belonging to the Scheduler.
/// Current thread is parent. Does NOT enqueue into any scheduling queue.
thread *thread_spawn(thread *me, Scheduler *sched,
                     thread_func f, void *arg) {
  assert( current_thread == me );
  thread *thr = (thread*)malloc(sizeof(thread));
  thr->co = coro_spawn(me->co, tramp, STACK_SIZE);
  // Pass control to the trampoline a few times quickly to set up
  // the call of <f>.  Could also create a struct with all this data?
  current_thread = thr;
  coro_invoke(me->co, thr->co, (void *)me->co);
  coro_invoke(me->co, thr->co, thr);
  coro_invoke(me->co, thr->co, (void *)f);
  coro_invoke(me->co, thr->co, (void *)arg);
  current_thread = me;
  
  thr->next = NULL;
  thr->done = 0;
  
  thr->sched = sched;
  sched->assignTid( thr );
  
  return thr;
}


void Scheduler::run ( ) {
  while (thread_wait(sched, NULL) != NULL) { } // nothing
}

void thread_join(thread* me, thread* wait_on) {
    if (!wait_on->done) {
        wait_on->joinlist.enqueue( me );
        thread_suspend( me );
    }
}



void thread_exit(thread *me, void *retval) {
  thread *master = me->sched->master;

  // Reuse the queue field for a return value.
  me->next = (thread *)retval;

  // process join
  me->done = 1;
  thread* waiter = join_list_dequeue(me);
  while (waiter!=NULL) {
    thread_wake(waiter);
    waiter = join_list_dequeue(me);
  }
  
  // call master thread
  current_thread = master;
  coro_invoke(me->co, master->co, (void *)me);
  // never returns
  exit(EXIT_FAILURE);
}

thread *thread_wait(scheduler *sched, void **result) {
  thread *next = scheduler_dequeue(sched);
  if (next == NULL) {
    return NULL;
  } else {
    current_thread = next;
    thread *died = (thread *)coro_invoke(sched->master->co,
                                         next->co, NULL);
    // At the moment, we only return from a thread in the case of death.
    // That might change as we support synchronization/related features.
   
    if (result != NULL) {
      void *retval = (void *)died->next;
      *result = retval;
    }
    return died;
  }
}


void destroy_thread(thread *thr) {
  destroy_coro(thr->co);
  free (thr);
}

void destroy_scheduler(scheduler *sched) {
  thread *thr;
  while ((thr = scheduler_dequeue(sched))) {
    destroy_thread(thr);
  }
  free(sched);
}
