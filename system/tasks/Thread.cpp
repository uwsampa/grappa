#include "Thread.hpp"
#include <cstdlib>
#include <cassert>
#include "Scheduler.hpp"

#define STACK_SIZE 2<<18

thread * thread_init() {
  coro* me = coro_init();
  thread* master = (thread*)malloc(sizeof(thread));
  assert(master != NULL);
  master->co = me;
  master->sched = NULL;
  master->next = NULL;
  master->id = 0; // master always id 0
  master->done = 0;
  
  return master;
}

static void tramp(struct coro * me, void * arg) {
  // Pass control back and forth a few times to get the info we need.
  coro* master = (coro *)arg;
  thread* my_thr = (thread *)coro_invoke(me, master, NULL);
  thread_func f = (thread_func)coro_invoke(me, master, NULL);
  void* f_arg = coro_invoke(me, master, NULL);
  // Next time we're invoked, it'll be for real.
  coro_invoke(me, master, NULL);

  f(my_thr, f_arg);

  // We shouldn't return, but if we do, kill the thread.
  thread_exit(my_thr, NULL);
}

/// Spawn a new thread belonging to the Scheduler.
/// Current thread is parent. Does NOT enqueue into any scheduling queue.
thread * thread_spawn(thread * me, Scheduler * sched,
                     thread_func f, void * arg) {
  assert( sched->get_current_thread() == me );
 
  // allocate the thread and stack 
  thread* thr = (thread*)malloc(sizeof(thread));
  thr->co = coro_spawn(me->co, tramp, STACK_SIZE);

  // Pass control to the trampoline a few times quickly to set up
  // the call of <f>.  Could also create a struct with all this data?
  
  //current_thread = thr; //legacy: unnecessary to set current thread
  
  coro_invoke(me->co, thr->co, (void *)me->co);
  coro_invoke(me->co, thr->co, thr);
  coro_invoke(me->co, thr->co, (void *)f);
  coro_invoke(me->co, thr->co, (void *)arg);
  
  //current_thread = me; //legacy: unnecessary to set current thread
  
  thr->next = NULL;
  thr->done = 0;
  
  thr->sched = sched;
  sched->assignTid( thr );
  
  return thr;
}

void thread_exit(thread * me, void * retval) {

  // Reuse the queue field for a return value.
  me->next = (thread *)retval;

  // process join
  me->done = 1;
  thread* waiter = me->joinqueue.dequeue();
  while (waiter!=NULL) {
    me->sched->thread_wake( waiter );
    waiter = me->joinqueue.dequeue();
  }
  
  // call master thread
  me->sched->thread_on_exit();
 
  // never returns
  exit(EXIT_FAILURE);
}

void destroy_thread(thread * thr) {
  destroy_coro(thr->co);
  free (thr);
}

