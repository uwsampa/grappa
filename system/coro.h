#ifndef CORO_H
#define CORO_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "stack.h"

/* might add more fields later */
typedef struct coro {
  int running;
  // start of stack
  void *base;
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
