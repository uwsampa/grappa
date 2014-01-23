////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef VTRACE
#include <vt_user.h>
#endif

#include "stack.h"
#include "StateTimer.hpp"
#include "PerformanceTools.hpp"
#include "Addressing.hpp"

#ifdef ENABLE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <glog/logging.h>

namespace Grappa { class Scheduler; }

typedef uint32_t threadid_t;
#ifdef GRAPPA_TRACE
// keeps track of last id assigned
extern int thread_last_tau_taskid;
#endif

/// Size in bytes of the stack allocated for every Worker
// const size_t STACK_SIZE = 1L<<19;
DECLARE_int64(stack_size);
#define STACK_SIZE FLAGS_stack_size

#include <sys/mman.h> // mprotect
#include <errno.h>

namespace Grappa {

/// Worker/coroutine
class Worker {
  //TODO
  //enum State { RUNNING, SUSPENDED };

  public: // TODO: privatize some members
    Worker() {}

  /* used on a context switch */
  // current stack pointer.  Since stack grows down in x86,
  // stack >= base (hopefully!)
  void * stack;
  
  /* used on synchronization objects (and maybe switch?) */
  // next thread field for wait queues
  Worker * next;

  /* use for just assertions? */
  // status flags; bit fields to save space
  //TODO State runstate;
  union {
    struct {
      int running : 1;
      int suspended : 1;
      int idle : 1;
    };
    int8_t run_state_raw_;
  };

  /* used at startup and shutdown */
  Scheduler * sched; 
  bool done;

  /* used less often */
  // start of the stack
  void * base;
  // size of the stack
  size_t ssize;
  threadid_t id;

  /* debugging state */
#ifdef CORO_PROTECT_UNUSED_STACK
  void *guess;
#endif

#ifdef ENABLE_VALGRIND
  // valgrind
  unsigned valgrind_stack_id;
#endif

  // pointers for tracking all coroutines
  Worker * tracking_prev;
  Worker * tracking_next;

#ifdef GRAPPA_TRACE
  int state;
  int tau_taskid;
#endif
 
  public: 
  inline intptr_t stack_remaining() {
#ifdef __clang__
    int64_t rsp = 0;
    asm volatile("mov %%rsp, %0" : "=r"(rsp) );
#else
    register long rsp asm("rsp");
#endif
    int64_t remain = static_cast<int64_t>(rsp) - reinterpret_cast<int64_t>(this->base) - 4096;
    DCHECK_LT(remain, static_cast<int64_t>(STACK_SIZE)) << "rsp = " << reinterpret_cast<void*>(rsp) << ", base = " << base << ", STACK_SIZE = " << STACK_SIZE;
    DCHECK_GE(remain, 0) << "rsp = " << reinterpret_cast<void*>(rsp) << ", base = " << base << ", STACK_SIZE = " << STACK_SIZE;

    return remain;
  }
  
  /// prefetch the Worker execution state
  inline void prefetch() {
    __builtin_prefetch( stack, 0, 3 ); // try to keep stack in cache
    __builtin_prefetch( next, 0, 3 ); // try to keep next worker in cache

    //if( data_prefetch ) __builtin_prefetch( data_prefetch, 0, 0 );   // for low-locality data
  }
  } GRAPPA_BLOCK_ALIGNED;

namespace impl {

void checked_mprotect( void *addr, size_t len, int prot );


typedef void (*thread_func)(Worker *, void *arg);

/// Turns the current (system) thread into a Grappa Worker.
/// This is simply necessary so that a Scheduler can
/// cause a context switch back to the system thread when
/// the Scheduler is done
Worker * convert_to_master( Worker * me = NULL );

/// spawn a new coroutine, creating a stack and everything, but
/// doesn't run until scheduled
void coro_spawn(Worker * me, Worker * c, coro_func f, size_t ssize);

/// pass control to <to> (giving it <val>, either as an argument for a
/// new coro or the return value of its last invoke.)
static inline void * coro_invoke(Worker * me, Worker * to, void * val) {
#ifdef CORO_PROTECT_UNUSED_STACK
  if( to->base != NULL ) {
    checked_mprotect( (void*)((intptr_t)to->base + 4096), to->ssize, PROT_READ | PROT_WRITE );
    checked_mprotect((void*)(to), 4096, PROT_READ | PROT_WRITE );
  }

  // compute expected stack pointer

  // get current stack pointer
  void * rsp = NULL;
  asm volatile("mov %%rsp, %0" : "=r"(rsp) );

  // adjust by register count
  // TODO: couple this with save/restore strategies in stack.S
  int num_registers_to_save = 8; 
  intptr_t stack_with_regs = (intptr_t)rsp - 8*1 - 8*num_registers_to_save; // 8-byte PC + 8-byte saved registers
  me->guess = (void*) stack_with_regs; // store for debugging

  // round down to 4KB pages
  int64_t size = (stack_with_regs - ((intptr_t)me->base + 4096)) & 0xfffffffffffff000L;
  if( me != to &&           // don't protect if we're switching to ourselves
      me->base != NULL &&   // don't protect if it's the native host thread
      size > 0 ) {
    checked_mprotect( (void*)((intptr_t)me->base + 4096), size, PROT_READ );
    checked_mprotect( (void*)(me), 4096, PROT_READ );
  }
#endif

  me->running = 0;
  to->running = 1;

  val = swapstacks_inline(&(me->stack), &(to->stack), val);
  return val;
}

/// Called when a Worker completes its function.
/// Worker->next will contain retval.
/// This function does not return to the caller.
void thread_exit(Worker * me, void * retval);


/// Trampoline for spawning a new thread.
static void tramp(Worker * me, void * arg) {
  // Pass control back and forth a few times to get the info we need.
  Worker * master = (Worker *) arg;
  Worker * my_thr = (Worker *) coro_invoke(me, master, NULL);
  thread_func f = (thread_func) coro_invoke(me, master, NULL);
  void * f_arg = coro_invoke(me, master, NULL);
  // Next time we're invoked, it'll be for real.
  coro_invoke(me, master, NULL);

  StateTimer::setThreadState( StateTimer::SYSTEM );
  StateTimer::enterState_system();
  
  // create new Tau task, and top level timer for the task
#ifdef GRAPPA_TRACE
  int new_taskid;
  TAU_CREATE_TASK(new_taskid);
  my_thr->tau_taskid = new_taskid;
  thread_last_tau_taskid = new_taskid;
  GRAPPA_PROFILE_CREATE( mainprof, "start_thread", "()", TAU_DEFAULT );
  GRAPPA_PROFILE_THREAD_START( mainprof, my_thr );
#endif

  // call thread target function
  f(my_thr, f_arg);

  // stop top level Tau task timer
  GRAPPA_PROFILE_THREAD_STOP( mainprof, my_thr );

  // We shouldn't return, but if we do, kill the Worker.
  thread_exit(my_thr, NULL);
}


/// Spawn a new Worker belonging to the Scheduler.
/// Current Worker is parent. Does NOT enqueue into any scheduling queue.
Worker * worker_spawn(Worker * me, Scheduler * sched,
                     thread_func f, void * arg);

/// Tear down a coroutine
void destroy_coro(Worker * c);

/// Delete the thread.
void destroy_thread(Worker * thr);


/// Perform a context switch to another Worker
/// @param running the current Worker
/// @param next the Worker to switch to
/// @param val pass a value to next
inline void * thread_context_switch( Worker * running, Worker * next, void * val ) {
    // This timer ensures we are able to calculate exclusive time for the previous thing in this thread's callstack,
    // so that we don't count time in another thread
    GRAPPA_THREAD_FUNCTION_PROFILE( GRAPPA_SUSPEND_GROUP, running );  
#ifdef VTRACE_FULL
  VT_TRACER("context switch");
#endif
    void * res = coro_invoke( running, next, val );
    StateTimer::enterState_thread();
    return res; 
}

} // namespace impl
} // namespace Grappa
