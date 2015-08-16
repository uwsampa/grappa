////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "stack.h"
#include "Worker.hpp"
#include "Scheduler.hpp"
#include "PerformanceTools.hpp"
#include <stdlib.h> // valloc
#include "LocaleSharedMemory.hpp"

DEFINE_int64( stack_size, MIN_STACK_SIZE, "Default stack size" );

namespace Grappa {
namespace impl {

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
  me->idle = 0;
  
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

#ifdef CORO_PROTECT_UNUSED_STACK
  // disable writes to stack until we're swtiched in again.
  //assert( 0 == mprotect( (void*)((intptr_t)me->base + 4096), ssize, PROT_READ ) );
  assert( 0 == mprotect( (void*)(me), 4096, PROT_READ ) );
#endif

  return me;
}

#include <errno.h>
void coro_spawn(Worker * me, Worker * c, coro_func f, size_t ssize) {
  CHECK(c != NULL) << "Must provide a valid Worker";
  c->running = 0;
  c->suspended = 0;
  c->idle = 0;

  // allocate stack and guard page
  c->base = Grappa::impl::locale_shared_memory.allocate_aligned( ssize+4096*2, 4096 );
  CHECK_NOTNULL( c->base );
  c->ssize = ssize;

  // set stack pointer
  c->stack = (char*) c->base + ssize + 4096 - current_stack_offset;

  // try to make sure we don't stuff all our stacks at the same cache index
  const int num_offsets = 128;
  const int cache_line_size = 64;
  current_stack_offset += FLAGS_stack_offset;
  current_stack_offset &= ((cache_line_size * num_offsets) - 1);
  
  c->tracking_prev = NULL;
  c->tracking_next = NULL;

#ifdef ENABLE_VALGRIND
  c->valgrind_stack_id = VALGRIND_STACK_REGISTER( (char *) c->base + 4096, c->stack );
#endif

  // clear stack
  memset(c->base, 0, ssize+4096*2);

#ifdef GUARD_PAGES_ON_STACK
  // arm guard page
  checked_mprotect( c->base, 4096, PROT_NONE );
  checked_mprotect( (char*)c->base + ssize + 4096, 4096, PROT_NONE );
#endif

  // set up coroutine to be able to run next time we're switched in
  makestack(&me->stack, &c->stack, f, c);
  
  insert_coro( c ); // insert into debugging list of coros

#ifdef CORO_PROTECT_UNUSED_STACK
  // disable writes to stack until we're swtiched in again.
  checked_mprotect( (void*)((intptr_t)c->base + 4096), ssize, PROT_READ );
  checked_mprotect( (void*)(c), 4096, PROT_READ );
#endif

  total_coros++;
}

// TODO: refactor not to take <me> argument
Worker * worker_spawn(Worker * me, Scheduler * sched, thread_func f, void * arg) {
  CHECK( sched->get_current_thread() == me ) << "parent arg differs from current thread";
 
  // allocate the Worker and stack
  Worker * thr = nullptr;
  CHECK( posix_memalign( reinterpret_cast<void**>( &thr ), 4096, sizeof(Worker) ) == 0)
    << "posix_memalign error: Worker allocation failed";
  
  thr->sched = sched;
  sched->assignTid( thr );
  
  coro_spawn(me, thr, tramp, FLAGS_stack_size);


  // Pass control to the trampoline a few times quickly to set up
  // the call of <f>.  Could also create a struct with all this data?
  
  coro_invoke(me, thr, (void *)me);
  coro_invoke(me, thr, thr);
  coro_invoke(me, thr, (void *)f);
  coro_invoke(me, thr, (void *)arg);
  
  
  return thr;
}

void destroy_coro(Worker * c) {
  total_coros--;
#ifdef ENABLE_VALGRIND
  if( c->valgrind_stack_id != -1 ) {
    VALGRIND_STACK_DEREGISTER( c->valgrind_stack_id );
  }
#endif
  if( c->base != NULL ) {
    // disarm guard page
    checked_mprotect( c->base, 4096, PROT_READ | PROT_WRITE );
    checked_mprotect( (char*)c->base + c->ssize + 4096, 4096, PROT_READ | PROT_WRITE );
#ifdef CORO_PROTECT_UNUSED_STACK
    // enable writes to stack so we can deallocate
    checked_mprotect( (void*)((intptr_t)c->base + 4096), c->ssize, PROT_READ | PROT_WRITE );
    checked_mprotect( (void*)(c), 4096, PROT_READ | PROT_WRITE );
#endif
    remove_coro(c); // remove from debugging list of coros
    Grappa::impl::locale_shared_memory.deallocate(c->base);
  }
}

void destroy_thread(Worker * thr) {
  destroy_coro(thr);
  free (thr);
}

void thread_exit(Worker * me, void * retval) {

  // Reuse the queue field for a return value.
  me->next = (Worker *)retval;

  // call master Worker
  me->sched->thread_on_exit();
 
  // never returns
  exit(EXIT_FAILURE);
}

void checked_mprotect( void *addr, size_t len, int prot ) {
  CHECK( 0 == mprotect( addr, len, prot ) )
    << "mprotect failed" 
    << "; addr= "<< addr 
    << "; len= " << len
    << "; errno=" << errno << "; "
    << ((errno == EINVAL) ? "errno==EINVAL (addr not a valid pointer or not a multiple of the system page size)"
    : (errno == ENOMEM) ? "errno==ENOMEM (internal kernel structures could not be allocated OR invalid addresses in range"
    : "(unrecognized)");
}

} // namespace impl
} // namespace Grappa
