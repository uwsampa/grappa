
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef CORO_H
#define CORO_H
#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdint.h>
#include "stack.h"
#include <stdio.h>

#ifdef ENABLE_VALGRIND
#include <valgrind/valgrind.h>
#endif

/// worker/coroutine implementation
/// TODO: merge threads and coroutines to make "workers"

/// Coroutine struct
typedef struct coro {
  int running;
  int suspended;
  bool idle;
  // start of stack
  void *base;
  size_t ssize;
#ifdef CORO_PROTECT_UNUSED_STACK
  void *guess;
#endif
  // current stack pointer.  Since stack grows down in x86,
  // stack >= base (hopefully!)
  void *stack;

#ifdef ENABLE_VALGRIND
  // valgrind
  unsigned valgrind_stack_id;
#endif

  // pointers for tracking all coroutines
  struct coro * prev;
  struct coro * next;
} coro;

/// Turn the currently-running pthread into a "special" coroutine.
/// This coroutine is used only to execute spawned coroutines.
coro *coro_init();

/// spawn a new coroutine, creating a stack and everything, but
/// doesn't run until scheduled
coro *coro_spawn(coro *me, coro_func f, size_t ssize);

/// pass control to <to> (giving it <val>, either as an argument for a
/// new coro or the return value of its last invoke.)
static inline void *coro_invoke(coro *me, coro *to, void *val) {
#ifdef CORO_PROTECT_UNUSED_STACK
  if( to->base != NULL ) {
    assert( 0 == mprotect( (void*)((intptr_t)to->base + 4096), to->ssize, PROT_READ | PROT_WRITE ) );
    assert( 0 == mprotect( (void*)(to), 4096, PROT_READ | PROT_WRITE ) );
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
    assert( 0 == mprotect( (void*)((intptr_t)me->base + 4096), size, PROT_READ ) );
    assert( 0 == mprotect( (void*)(me), 4096, PROT_READ ) );
  }
#endif

  me->running = 0;
  to->running = 1;

  val = swapstacks_inline(&(me->stack), &(to->stack), val);
  return val;
}

/// Tear down a coroutine
void destroy_coro(coro *c);

#ifdef __cplusplus
}
#endif
#endif
