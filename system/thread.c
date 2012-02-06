#include "thread.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define STACK_SIZE 1<<22

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
  current_thread = master;
  return master;
}

scheduler *create_scheduler(thread *master) {
  scheduler *sched = (scheduler*)malloc(sizeof(scheduler));
  assert(sched != NULL);
  sched->ready = NULL;
  sched->tail = NULL;
  sched->wait = NULL;
  sched->wait_tail = NULL;
  sched->master = master;
  sched->nextId = 1; //non-master id starts at 1
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
  thr->sched = sched;
  thr->next = NULL;
  scheduler_assignTid(sched, thr);
  scheduler_enqueue(sched, thr);
  return thr;
}

void thread_exit(thread *me, void *retval) {
  thread *master = me->sched->master;

  // Reuse the queue field for a return value.
  me->next = (thread *)retval;
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

void run_all(scheduler *sched) {
  while (thread_wait(sched, NULL) != NULL) { } // nothing
}

void scheduler_assignTid(scheduler *sched, thread* thr) {
    thr->id = sched->nextId++;
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
