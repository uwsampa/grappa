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
/* might add more fields later */
typedef struct coro {
  int running;
  // start of stack
  void *base;
  size_t ssize;
#ifdef CORO_PROTECT_UNUSED_STACK
  void *guess;
#endif
  // current stack pointer.  Since stack grows down in x86,
  // stack >= base (hopefully!)
  void *stack;
} coro;

/* Turns the current thread into a coro. */
coro *coro_init();

/*
  allocates <ssize> bytes for stack.
  If <ssize> = 0, allocate default (16k).
  Does not run result, but when invoked, calls <f>.
*/
coro *coro_spawn(coro *me, coro_func f, size_t ssize);

/* pass control to <to> (giving it <val>, either as an argument for a
 * new coro or the return value of its last invoke.) */
static inline void *coro_invoke(coro *me, coro *to, void *val) {
#ifdef CORO_PROTECT_UNUSED_STACK
  if( to->base != NULL ) assert( 0 == mprotect( (void*)((intptr_t)to->base + 4096), to->ssize, PROT_READ | PROT_WRITE ) );

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
  }
#endif

  me->running = 0;
  to->running = 1;

  val = swapstacks_inline(&(me->stack), &(to->stack), val);
  return val;
}

void destroy_coro(coro *c);

#ifdef __cplusplus
}
#endif
#endif
