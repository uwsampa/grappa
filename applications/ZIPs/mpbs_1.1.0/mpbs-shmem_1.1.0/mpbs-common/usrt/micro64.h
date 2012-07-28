// %Z% %P% 
// %Z% Delta=$Revision: 21509 $, Original=$Date: 2008-08-06 14:41:01 -0400 (Wed, 06 Aug 2008) $, Get=%H%

/*
* __CMT1 $Id: micro64.h 21509 2008-08-06 18:41:01Z pgacker $
*/

// This file includes typedefs for 8-, 16-, 32-, and 64-bit
// integer types, both signed and unsigned: [u]int[nn] where
// nn=8,16,32,64.  These are the smallest types which hold at
// least nn bits (e.g. int16 might actually be 4 bytes long if the
// particular architecture doesn't support a native 2 byte type.
//
// In addition we #define :
//   1) LITTLE_ENDIAN or BIG_ENDIAN
//   2) LONGIS32 on machines where sizeof(long)=4
//   3) _M64_INT_REGS = number of architectured integer registers
//

#ifndef MICRO64_H
#define MICRO64_H

// Let's deal with certain endianess problems first
#if ((defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)) || defined(__linux__) || defined(__GNU_LIBRARY__))
#if defined(__linux__) || defined(__GNU_LIBRARY__)
// GLIBC has <endian.h> which is not compatible
// with our endian defines.  Our fix is below: 
#include <endian.h>
#endif
#undef LITTLE_ENDIAN
#undef BIG_ENDIAN
#if   __BYTE_ORDER == __LITTLE_ENDIAN
#define _M64_LE
#elif __BYTE_ORDER == __BIG_ENDIAN
#define _M64_BE
#endif  // End: __BYTE_ORDER case
#endif  // End: (BE and LE) or linux or gnulib


// First we check for machines which have support
// for LP64 and/or ILP32 with no special handling.

// Dec Alpha/Tru64
#if defined(__alpha) && defined(__osf__)
#define _M64_LE
#define _M64_STD_LP64

// SGI
#elif defined(__sgi)
#define _M64_BE
#if (_MIPS_SIM == _MIPS_SIM_ABI64)
#define _M64_STD_LP64
#else
#define _M64_STD_ILP32
#endif

// Sun
#elif defined(__sparc)
#define _M64_BE
#ifdef __sparcv9
#define _M64_STD_LP64
#else
#define _M64_STD_ILP32
#endif


// Solaris/x86 
#elif defined(__sun)
#define _M64_LE
#if   defined(__x86_64)
#define _M64_STD_LP64
#elif defined(__i386)
#define _M64_STD_ILP32
#else
#error "MICRO64 ERROR: no definitions for this __sun architecture."
#endif

// HP-UX
#elif defined(__hpux)
#define _M64_BE
#ifdef __LP64__
#define _M64_STD_LP64
#else
#define _M64_STD_ILP32
#endif

// for Linux (x86, IA64, Alpha) 
#elif defined(__linux__) || defined(__GNU_LIBRARY__)
// endianess for these cases was handled at the top of the file

#if defined(__ia64__) || defined(__alpha) || defined(__x86_64) || defined(__x86_64__) || (defined(__WORDSIZE) && (__WORDSIZE == 64))
#define _M64_STD_LP64
#else
#define _M64_STD_ILP32
#endif

// The machines below have some quirks which require
// special handling (currently Cray and IBM)

// Cray (T3D, T3E, PVP)
#elif defined(_CRAY)
#define _M64_BE
#ifdef _CRAY1
// T90, SV1 
#define NOT_BYTE_ADDRESSED
#endif
#if defined (_CRAY1) || defined(_CRAYMPP)
// T3E, T90, SV1 
typedef   signed  char  int8 ;
typedef unsigned  char uint8 ;
typedef          short  int16;
typedef unsigned short uint16;
typedef          short  int32;
typedef unsigned short uint32;
typedef           long  int64;
// uint64 is already defined on Cray (via /usr/include/sys/types.h)
// if __SYS_TYPES_H_ is defined and _POSIX_SOURCE is not defined.
#if !defined(__SYS_TYPES_H_) || defined(_POSIX_SOURCE)
typedef unsigned  long uint64;
#endif
#else // Cray X1 
typedef   signed  char  int8 ;
typedef unsigned  char uint8 ;
typedef          short  int16;
typedef unsigned short uint16;
typedef            int  int32;
typedef unsigned   int uint32;
typedef           long  int64;
typedef unsigned  long uint64;
#endif

// for IBM 
#elif defined(_AIX)
#define _M64_BE
// inttypes.h in AIX already provides our signed
// typedefs so we include it (to avoid conflicts)
// and just define our unsigned types.
#include <inttypes.h>
typedef unsigned  char uint8 ;
typedef unsigned short uint16;
typedef unsigned   int uint32;
#ifdef __64BIT__
typedef unsigned  long uint64;
#else
#define LONGIS32
typedef unsigned long long uint64;
#endif

#else

#error MICRO64 ERROR: no definitions for this architecture.

#endif // End: different types of architectures

#ifdef _M64_STD_LP64
typedef   signed  char  int8 ;
typedef unsigned  char uint8 ;
typedef          short  int16;
typedef unsigned short uint16;
typedef            int  int32;
typedef unsigned   int uint32;
typedef           long  int64;
typedef unsigned  long uint64;

#elif defined(_M64_STD_ILP32)
#define LONGIS32
typedef   signed      char  int8 ;
typedef unsigned      char uint8 ;
typedef              short  int16;
typedef unsigned     short uint16;
typedef                int  int32;
typedef unsigned       int uint32;
typedef          long long  int64;
typedef unsigned long long uint64;

#endif

// define endianness based on observations above
#if   defined(_M64_BE)
#define BIG_ENDIAN
#elif defined(_M64_LE)
#define LITTLE_ENDIAN
#endif

// Check for errors in ENDIAN #defines
#if defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#error MICRO64 ERROR: both BIG_ENDIAN and LITTLE_ENDIAN are defined.
#endif
#if (! (defined(BIG_ENDIAN) || defined(LITTLE_ENDIAN)))
#error MICRO64 ERROR: neither BIG_ENDIAN nor LITTLE_ENDIAN are defined.
#endif

// These macros take care of the long vs. long long problem
// for integer constants;  we use concatenation macros
#ifdef LONGIS32  // 64-bit challenged machines 
#define  I64(c)  (c##LL)
#define UI64(c)  (c##uLL)
#else
#define  I64(c)  (c##L)
#define UI64(c)  (c##uL)
#endif


// set number of architected registers
// we'll assume 32 unless we know otherwise 
#ifndef _M64_INT_REGS

#if   defined (_CRAY1)
// T90, SV1
#define _M64_INT_REGS 8

#elif defined(__i386)
// old x86
#define _M64_INT_REGS 8

#elif defined(__x86_64) || defined(__x86_64__)
// Opteron
#define _M64_INT_REGS 16

#elif defined(__ia64) || defined(__ia64__)
// Itanium
#define _M64_INT_REGS 128

#else
// Default 
#define _M64_INT_REGS 32

#endif
#endif // End: ndef _M64_INT_REGS


#endif // End: ndef MICRO64_H 

