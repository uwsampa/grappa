
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef STACK_H
#define STACK_H



class Worker;

typedef void (*coro_func)(Worker *, void *arg);

/// Swap stacks. Save context to <old>, restore context from <new> and
/// pass value <ret> to swapped-in stack.
void* swapstacks(void **olds, void **news, void *ret)
  asm ("_swapstacks");

// for now, put this here
// define this only when we're using the minimal save context switcher
// if you switch this off, you must update the save/restore settings in stack.S
//#define GRAPPA_SAVE_REGISTERS_LITE

// depend on compiler to save and restore callee-saved registers
static inline void* swapstacks_inline(void **olds, void **news, void *ret) {
  asm volatile ( ""
                 : "+a" (ret), "+D" (olds), "+S" (news) // add some dependences for ordering
                 : "D" (olds), "S" (news), "d" (ret)    // add some dependences for ordering
                 : "cc", "flags", 
#ifdef GRAPPA_SAVE_REGISTERS_LITE
                   "rbp", // doesn't work if we need a frame pointer
                   "rbx", "r12", "r13", "r14", "r15", "fpcr",
#endif
                   "memory"
                 );
  // warning: watch out for register restoration before the call
  return swapstacks( olds, news, ret );
}

/// Given memory going DOWN FROM <stack>, create a basic stack we can swap to
/// (using swapstack) that will call <f>. (using <it> as its <me>).
/// <me> is a location we can store the current stack.
void makestack(void **me, void **stack, coro_func f, Worker * it)
  asm ("_makestack");

#endif
