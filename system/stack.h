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
