#include "thread.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define STACK_SIZE 256

thread *thread_init() {
  coro *me = coro_init();
  thread *master = malloc(sizeof(thread));
  assert(master != NULL);
  master->coro = me;
  master->sched = NULL;
  master->next = NULL;
  return master;
}

scheduler *create_scheduler(thread *master) {
  scheduler *sched = malloc(sizeof(sched));
  assert(sched != NULL);
  sched->ready = NULL;
  sched->tail = NULL;
  sched->master = master;
  return sched;
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

thread *thread_spawn(thread *me, scheduler *sched,
                     thread_func f, void *arg) {
  thread *thr = malloc(sizeof(thread));
  thr->coro = coro_spawn(me->coro, tramp, STACK_SIZE);
  // Pass control to the trampoline a few times quickly to set up
  // the call of <f>.  Could also create a struct with all this data?
  coro_invoke(me->coro, thr->coro, (void *)me->coro);
  coro_invoke(me->coro, thr->coro, thr);
  coro_invoke(me->coro, thr->coro, (void *)f);
  coro_invoke(me->coro, thr->coro, (void *)arg);
  thr->sched = sched;
  thr->next = NULL;
  scheduler_enqueue(sched, thr);
  return thr;
}

void thread_exit(thread *me, void *retval) {
  thread *master = me->sched->master;

  // Reuse the queue field for a return value.
  me->next = (thread *)retval;
  coro_invoke(me->coro, master->coro, (void *)me);
  // never returns
  exit(EXIT_FAILURE);
}

thread *thread_wait(scheduler *sched, void **result) {
  thread *next = scheduler_dequeue(sched);
  
  if (next == NULL) {
    return NULL;
  } else {
    thread *died = (thread *)coro_invoke(sched->master->coro,
                                         next->coro, NULL);
    // At the moment, we only return from a thread in the case of death.
    // That might change as we support synchronization/related features.
    
    if (result != NULL) {
      void *retval = (void *)died->next;
      *result = retval;
    }
    return died;
  }
}

void run_all(scheduler *sched) {
  while (thread_wait(sched, NULL) != NULL) { } // nothing
}

void destroy_thread(thread *thr) {
  destroy_coro(thr->coro);
  free (thr);
}

void destroy_scheduler(scheduler *sched) {
  thread *thr;
  while ((thr = scheduler_dequeue(sched))) {
    destroy_thread(thr);
  }
}
