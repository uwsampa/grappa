#ifndef STACK_H
#define STACK_H



struct coro;

typedef void (*coro_func)(struct coro *, void *arg);

// move stacks from <old> to <new> and return <ret> (to new guy)
void* swapstacks(void **olds, void **news, void *ret);

// for now, put this here
// define this only when we're using the minimal save context switcher
// if you switch this off, you must update the save/restore settings in stack.S
#define SOFTXMT_SAVE_REGISTERS_LITE

// depend on compiler to save and restore callee-saved registers
static inline void* swapstacks_inline(void **olds, void **news, void *ret) {
  asm volatile ( ""
                 : "+a" (ret), "+D" (olds), "+S" (news) // add some dependences for ordering
                 : "D" (olds), "S" (news), "d" (ret)    // add some dependences for ordering
                 : "cc", "flags", 
#ifdef SOFTXMT_SAVE_REGISTERS_LITE
                   "rbp", // doesn't work if we need a frame pointer
                   "rbx", "r12", "r13", "r14", "r15", "fpcr",
#endif
                   "memory"
                 );
  // warning: watch out for register restoration before the call
  return swapstacks( olds, news, ret );
}

// Given memory going DOWN FROM <stack>, create a basic stack we can swap to
// (using swapstack) that will call <f>. (using <it> as its <me>).
// <me> is a location we can store the current stack.
void makestack(void **me, void **stack, coro_func f, struct coro *it);

#endif
