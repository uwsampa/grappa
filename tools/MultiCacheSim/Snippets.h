/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Milos Prvulovic
This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef SNIPPETS_H
#define SNIPPETS_H

#include <stdint.h>
//#if !(defined MIPS_EMUL)
//#include "mendian.h"
//#endif

//**************************************************
// Generic typedefs
typedef uint8_t        uchar;
typedef uint16_t       ushort;
typedef unsigned long  ulong;
typedef unsigned long long ulonglong;

//**************************************************
// Process typedefs

// CPU_t is signed because -1 means that it is not mapped to any CPU.
typedef int32_t CPU_t;

// Only the lower 16 bits are valid (at most 64K threads), but
// negative values may have special meaning (invalid == -1)
typedef int32_t         Pid_t;

//**************************************************
// Types used for time (move to callback?)
typedef unsigned long long Time_t;
const unsigned long long MaxTime = ((~0ULL) - 1024);  // -1024 is to give a little bit of margin

extern Time_t globalClock; // Defined in Thread.cpp

typedef uint16_t TimeDelta_t;
const uint16_t MaxDeltaTime = (65535 - 1024);  // -1024 is to give a little bit of margin

//**************************************************
// Memory subsystem
typedef intptr_t Address;

short  log2i(uint32_t n);

//x, y are integers and x,y > 0
#define CEILDiv(x,y)            ((x)-1)/(y)+1

uint32_t roundUpPower2(uint32_t x);

void debugAccess();

//******************** Prefetch macros
//
// Use the very wisely. Most of the cases instead of speeding up they
// slowdown the code. For example, a place like a queue, were a
// prefetch for a sequential element may be interesting
// (FastQueue:pop) makes a GLOBAL slowdown of 2%. Which is pretty
// bad!
// 
// Upto now, I only could find 3 places where the prefetch was useful
// in a pentium processor. Notice that it is not necessarely useful in
// another processor.
// 
// My two cents: 
//
// ONLY use prefetch when you are very certaint that there is a cache miss,
// and you can't reorganize the code so that the structure has more
// locality.
//
// ALSO ONLY use prefetch if you can known in advance (at least 40
// instructions) the address that you are going to require. If it is
// too much in advance (~1000 instructions) it is better not to do
// prefetch.
//
// Before considering prefetch try to reorganize the order in which
// the parameters of a class are used. This tends to reduce the cache
// miss without the extra cost of an additional instruction.
//
// ALWAYS verify that the Prefetch instructions realy makes an
// improvement. If the difference is less than 0.5% seriously consider
// to remove it.
//


#ifdef __GNUC__

#if defined(__tune_i686__)
#define Prefetch(addr) /* __asm__ __volatile__ ("prefetchnta (%0)" : : "r"((const void *)addr)) */
#elif defined(_ARCH_PPC)
#define Prefetch(addr) /* __asm__ __volatile__ ("dcbt 0,%0" : : "r"((const void*)addr)) */
#elif defined(__ia64__)
#define Prefetch(addr) /* __asm__ __volatile__ ("lfetch [%0]": : "r"((const void *)(addr))) */
#else
// This is a little trick from gcc. Rather than a prefetch is a real
// load, but it is better than nothing
// #define Prefetch(addr) __asm__ __volatile__ ("" : : "r"(addr))
#define Prefetch(addr) /* __asm__ __volatile__ ("" : : "r"(addr)) */
#endif /* __pentiumpro__ */

#else

#define Prefetch(addr) /* Do nothing */

#endif

#define K(n) ((n) * 1024)
#define M(n) (K(n) * 1024)
#define G(n) (M(n) * 1024)

#endif // SNIPPETS_H
