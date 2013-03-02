// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "stack.h"
#include "Worker.hpp"
#include "Scheduler.hpp"
#include "PerformanceTools.hpp"
#include <stdlib.h> // valloc
#include <sys/mman.h> // mprotect


/// list of all coroutines (used only for debugging)
Worker * all_coros = NULL;
/// total number of coroutines (used only for debugging)
size_t total_coros = 0;

DEFINE_int32( stack_offset, 64, "offset between coroutine stacks" );
size_t current_stack_offset = 0;



/// insert a coroutine into the list of all coroutines
/// (used only for debugging)
void insert_coro( Worker * c ) {
  // try to insert us in list going left
  if( all_coros ) {
    CHECK( all_coros->tracking_prev == NULL ) << "Head coroutine should not have a prev";
    all_coros->tracking_prev = c;
  }
    
  // insert us in list going right
  c->tracking_next = all_coros;
  all_coros = c;
}

/// remove a coroutine from the list of all coroutines
/// (used only for debugging)
void remove_coro( Worker * c ) {
  // is there something to our left?
  if( c->tracking_prev ) {
    // remove us from next list
    c->tracking_prev->tracking_next = c->tracking_next;
  }
  // is there something to our right?
  if( c->tracking_next ) {
    // remove us from next list
    c->tracking_next->tracking_prev = c->tracking_prev;
  }
}

#ifdef GRAPPA_TRACE
// keeps track of last id assigned
int thread_last_tau_taskid=0;
#endif


Worker * convert_to_master( Worker * me ) { 
  if (!me) me = new Worker();

  me->running = 1;
  me->suspended = 0;
  
  // We don't need to free this (it's just the main stack segment)
  // so ignore it.
  me->base = NULL;
  // This'll get overridden when we swapstacks out of here.
  me->stack = NULL;

#ifdef ENABLE_VALGRIND
  me->valgrind_stack_id = -1;
#endif

  // debugging state
  me->tracking_prev = NULL;
  me->tracking_next = NULL;

  total_coros++;
  insert_coro( me ); // insert into debugging list of coros

  me->sched = NULL;
  me->next = NULL;
  me->id = 0; // master is id 0 
  me->done = false;

#ifdef GRAPPA_TRACE 
  master->tau_taskid=0;
#endif

  return me;
}

void coro_spawn(Worker * me, Worker * c, coro_func f, size_t ssize) {
  CHECK(c != NULL) << "Must provide a valid Worker";
  c->running = 0;
  c->suspended = 0;

  // allocate stack and guard page
  c->base = valloc(ssize+4096*2); // TODO replace with supported function
  c->ssize = ssize;
  CHECK(c->base != NULL) << "Stack allocation failed";

  // set stack pointer
  c->stack = (char*) c->base + ssize + 4096 - current_stack_offset;
  current_stack_offset += FLAGS_stack_offset;
  
  c->tracking_prev = NULL;
  c->tracking_next = NULL;

#ifdef ENABLE_VALGRIND
  c->valgrind_stack_id = VALGRIND_STACK_REGISTER( (char *) c->base + 4096, c->stack );
#endif

  // clear stack
  memset(c->base, 0, ssize);

  // arm guard page
  CHECK( 0 == mprotect( (char*)c->base + ssize + 4096, 4096, PROT_NONE ) ) << "mprotect failed; check errno";

  // set up coroutine to be able to run next time we're switched in
  makestack(&me->stack, &c->stack, f, c);

#ifdef CORO_PROTECT_UNUSED_STACK
  // disable writes to stack until we're swtiched in again.
  CHECK( 0 == mprotect( (void*)((intptr_t)c->base + 4096), ssize, PROT_READ ) ) << "mprotect failed; check errno";
#endif

  total_coros++;
  insert_coro( c ); // insert into debugging list of coros
}

// TODO: refactor not to take <me> argument
Worker * worker_spawn(Worker * me, Scheduler * sched,
                     thread_func f, void * arg) {
  CHECK( sched->get_current_thread() == me ) << "parent arg differs from current thread";
 
  // allocate the Thread and stack
  Worker * thr = new Worker( );
  thr->sched = sched;
  sched->assignTid( thr );
  
  coro_spawn(me, thr, tramp, STACK_SIZE);


  // Pass control to the trampoline a few times quickly to set up
  // the call of <f>.  Could also create a struct with all this data?
  
  coro_invoke(me, thr, (void *)me);
  coro_invoke(me, thr, thr);
  coro_invoke(me, thr, (void *)f);
  coro_invoke(me, thr, (void *)arg);
  
  
  return thr;
}

void destroy_coro(Worker * c) {
  total_coros++;
  remove_coro(c); // remove from debugging list of coros
#ifdef ENABLE_VALGRIND
  if( c->valgrind_stack_id != -1 ) {
    VALGRIND_STACK_DEREGISTER( c->valgrind_stack_id );
  }
#endif
  if( c->base != NULL ) {
    // disarm guard page
    CHECK( 0 == mprotect( c->base, 4096, PROT_READ | PROT_WRITE ) ) << "mprotect failed; check errno";
    CHECK( 0 == mprotect( (char*)c->base + c->ssize + 4096, 4096, PROT_READ | PROT_WRITE ) ) << "mprotect failed; check errno";
#ifdef CORO_PROTECT_UNUSED_STACK
    // enable writes to stack so we can deallocate
    CHECK( 0 == mprotect( (void*)((intptr_t)c->base + 4096), c->ssize, PROT_READ | PROT_WRITE ) ) << "mprotect failed; check errno";
#endif
    free(c->base);
  }
}

void destroy_thread(Worker * thr) {
  destroy_coro(thr);
  free (thr);
}

void thread_exit(Worker * me, void * retval) {

  // Reuse the queue field for a return value.
  me->next = (Worker *)retval;

  // call master Thread
  me->sched->thread_on_exit();
 
  // never returns
  exit(EXIT_FAILURE);
}

/// ThreadQueue output stream
std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq ) {
    return tq.dump( o );
}
