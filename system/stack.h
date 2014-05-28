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

#ifndef STACK_H
#define STACK_H



namespace Grappa { class Worker; }

typedef void (*coro_func)(Grappa::Worker *, void *arg);

/// Swap stacks. Save context to `old`, restore context from `new` and
/// pass value `ret` to swapped-in stack.
void* swapstacks(void **olds, void **news, void *ret)
  asm ("_swapstacks");

// for now, put this here
// define this only when we're using the minimal save context switcher
// if you switch this off, you must update the save/restore settings in stack.S
//#define GRAPPA_SAVE_REGISTERS_LITE

// depend on compiler to save and restore callee-saved registers
static inline void* swapstacks_inline(void **olds, void **news, void *ret) {
//   asm volatile ( ""
//                  : "+a" (ret), "+D" (olds), "+S" (news) // add some dependences for ordering
//                  : "D" (olds), "S" (news), "d" (ret)    // add some dependences for ordering
//                  : "cc", "flags", 
// #ifdef GRAPPA_SAVE_REGISTERS_LITE
//                    "rbp", // doesn't work if we need a frame pointer
//                    "rbx", "r12", "r13", "r14", "r15", "fpcr",
// #endif
//                    "memory"
//                  );
  asm volatile ( "" : : : "memory" );
  // warning: watch out for register restoration before the call
  return swapstacks( olds, news, ret );
}

/// Given memory going DOWN FROM `stack`, create a basic stack we can swap to
/// (using swapstack) that will call `f`. (using `it` as its `me`).
/// `me` is a location we can store the current stack.
void makestack(void **me, void **stack, coro_func f, Grappa::Worker * it)
  asm ("_makestack");

#endif
