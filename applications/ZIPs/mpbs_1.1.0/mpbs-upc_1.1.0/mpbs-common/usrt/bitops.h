// macros/functions for  some CRAY intrinsics 

//
// $Id: bitops.h 26786 2011-03-07 16:23:14Z klfarre $
//

#include <micro64.h>

#ifndef BITOPS_H
#define BITOPS_H

//                       SECTION 1a:                        
//                                                          
//   Inlining directives for different machines/compilers   
//                                                          
//   (we check for C99 last as in some cases the compiler-  
//    specific directives give better performance        )  

#ifndef INLINE
// we check for specific compilers first
#if   defined(__DECC)
#define INLINE __inline
#elif defined(__INTEL_COMPILER)
#define INLINE inline
#elif defined(__IBMC__) || defined(__IBMCPP__)
#define INLINE __inline
#elif defined(__PGI)
#define INLINE inline
#elif defined(__UPC__)
#define INLINE inline
#elif defined(__GNUC__)
#define INLINE __inline__
// then for specific OS's
#elif defined(__hpux)
#define INLINE __inline
#elif defined(__sgi)
#define INLINE __inline
// finally we check for C99 (see performance note above)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define INLINE inline
#else
#define INLINE
#endif
#endif // End: ndef INLINE 

//                       SECTION 1b:                        
//                                                          
//   restricted pointers for different machines/compilers   
//                                                          

#ifndef RESTRICT
// we check for specific compilers first
#if   defined(__DECC)
#define RESTRICT __restrict
#elif defined(__INTEL_COMPILER)
#define RESTRICT restrict
#elif defined(__PGI)
#define RESTRICT restrict
#elif defined(_CRAYC)
#define RESTRICT restrict
#elif defined(__GNUC__)
#define RESTRICT __restrict__
// then for specific OS's
#elif defined(__hpux)
#define RESTRICT __restrict
#elif defined(__sparc)
#define RESTRICT _Restrict
// finally we check for C99 (see performance note above)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define RESTRICT restrict
#else
#define RESTRICT
#endif
#endif // End: ndef RESTRICT

//
//  End: SECTION 1


//                       SECTION 2:                         
//                                                          
//        Define CRI instrinsics for all systems            
//                                                          

#ifndef _CRAYC

// on architectures which added popcnt later in their life
// we will use (BITOPS_HW_POPCNT>0) to check if it is available
//
// on architectures which added leadz later in their life
// we will use (BITOPS_HW_LEADZ>0) to check if it is available

// Here we set some constants used below. UI64 is a macro   
// from bitops.h. It concatenates uL or uLL; the latter is  
// required for 64-bit challenged machines.                 

#define _MASK1BIT  UI64(0x5555555555555555)
#define _MASK2BITS UI64(0x3333333333333333)
#define _MASK4BITS UI64(0x0f0f0f0f0f0f0f0f)
#define _ZERO64    UI64(0x0)


//                       SECTION 2a:                        
//                                                          
//                   Legacy intrinsics                      
//                                                          

//   No special hardware for these operations so must be    
// explicitly enabled (suggested use is only for porting    
// legacy code).                                            
#ifdef _CRI_COMPATIBLE_

// beta for remaining CRI intrinsics                        
#define _gbit(x,n)       ((((uint64)(x))>>(n))&        1)
#define _gbits(x,l,n)    ((((uint64)(x))>>(n))&_maskr(l))

#define _pbit(x,n,y)     ((((uint64)(x)) & ~(   I64(1)<<(n)))|      \
			 ((((uint64)(y))&         1) << (n)))
#define _pbits(x,l,n,y)  ((((uint64)(x)) & ~(_maskr(l)<<(n)))|      \
			 ((((uint64)(y))& _maskr(l)) << (n)))

#define _dshiftl(x,y,n)  (((n) == 0) ? (x) : (((n) == 64) ? (y) :   \
			 ((((uint64)(x))<<(n))|(((uint64)(y))>>(64-(n))))))
#define _dshiftr(x,y,n)  (((n) == 0) ? (y) : (((n) == 64) ? (x) :   \
			 ((((uint64)(y))>>(n))|(((uint64)(x))<<(64-(n))))))

#endif // _CRI_COMPATIBLE_ 

//                       SECTION 2b:                        
//                                                          
//               Fully-supported intrinsics                 
//                                                          

// MASK macros 
#define _maskl(x)        (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64-(x))))
#define _maskr(x)        (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64-(x))))
#define _mask(x)         (((x) < 64) ? _maskl(x) : _maskr(2*64 - (x)))

// New DEC compiler supports these intrinsics               
#if   defined(__DECC_VER  ) && (__DECC_VER   >= 50730034)
#include <machine/builtins.h>

// So does DEC C++ 
#elif defined(__DECCXX_VER) && (__DECCXX_VER >= 50730034)
#include <machine/builtins.h>

#elif defined(__alpha) && defined(__GNUC__) && !defined(_CRAY)

static INLINE int64 _popcnt (uint64 x)

{
  int64 ans;
  
  asm ("ctpop %1,%0" : "=r" (ans) : "r" (x));
  return ans;
}

#define _poppar(x)  (_popcnt(x)&1)

static INLINE int64 _leadz (uint64 x)

{
  int64 ans;

  asm ("ctlz %1,%0"  : "=r" (ans) : "r" (x));
  return ans;
}

static INLINE int64 _trailz (uint64 x)

{
  int64 ans;

  asm ("cttz %1,%0"  : "=r" (ans) : "r" (x));
  return ans;
}

// Itanium-specific stuff 
#elif defined(__ia64) || defined(__ia64__)

#if   defined(__INTEL_COMPILER)
#include <ia64intrin.h>
//#include <xmmintrin.h>
#define _popcnt    _m64_popcnt

#elif defined(__hpux)

#include <machine/sys/inline.h>
#define _popcnt    _Asm_popcnt

#elif defined(__GNUC__)
static INLINE int64 _popcnt (uint64 x)

{
  int64 ans;
  
  asm ("popcnt %0=%1" : "=r" (ans) : "r" (x));
  return ans;
}

#endif // End: which IA64 compiler 

#define _poppar(x) (_popcnt(x)&1)

static INLINE int64 _leadz (uint64 x)

{
#if defined(__INTEL_COMPILER)
  return x ?
    (0x1003E - _Asm_getf(3,_Asm_fnorm(3,_Asm_setf(4,x))))
    : 64;
  
#elif defined(__hpux)
  return x ?
    (0x1003E - _Asm_getf(_FR_EXP,_Asm_fnorm(_PC_NONE,_Asm_setf(_FR_SIG,x))))
    : 64;

#else
  // propogate 1's 
  x |= x >>  1;
  x |= x >>  2;
  x |= x >>  4;
  x |= x >>  8;
  x |= x >> 16;
  x |= x >> 32;

  return _popcnt(~x);
#endif
}

static INLINE int64 _trailz (uint64 x)

{
  return _popcnt ((~x) & (x-1)); // has most ILP  
  // old way:       return _popcnt (~(x | -x));   
  // equiv to old:  return 64 - _popcnt (x | -x); 
  // oldest way:    return (x == 0) ? 64 : (_popcnt (x ^ (x-1)) - 1); 
}

// Power/PowerPC 
#elif defined(__PPC__) || defined(__POWERPC__) || defined(__ppc__) || defined(__powerpc__)

#if defined(__IBMC__) || defined(__IBMCPP__)

#define _popcnt(X)  ((int64)__popcnt8(X))
#define _poppar(X)  ((int64)__poppar8(X))
#define _leadz(X)   ((int64)__cntlz8(X))
#define _trailz(X)  ((int64)__cnttz8(X))

#elif defined(__GNUC__)

static INLINE int64 _popcnt (uint64 x)

{
  int64 ans;
  
  asm ("popcntd %0, %1" : "=r" (ans) : "r" (x));
  return ans;
}

static INLINE int64 _poppar (uint64 x)

{
#if 0
  // Note (Mar '11):
  // Still investigating which code sequence should be better
  // Currently popcnt&1 is faster in my testing with GCC 4.3.4
  int64 ans;
  
  asm ("popcntb %0, %1 ; prtyd %0,%0" : "=r" (ans) : "r" (x));
  return ans;
#else
  return _popcnt(x)&1;
#endif
}

static INLINE int64 _leadz (uint64 x)

{
  int64 ans;
  
  asm ("cntlzd %0, %1" : "=r" (ans) : "r" (x));
  return ans;
}

static INLINE int64 _trailz (uint64 x)

{
  // TODO: is this the best way on Power7 ??
  return _popcnt ((~x) & (x-1)); // has most ILP  
}

#endif // End: which PowerPC compiler   

// x86-64 with GCC, ICC, or PGI (also PathScale which asserts __GNUC__)
#elif (defined(__x86_64) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__PGI))

#if defined(BITOPS_HW_POPCNT) && (BITOPS_HW_POPCNT > 0)  // x86 with popcnt...

// ...gives us optimized _popcnt, _poppar, _trailz

static INLINE int64 _popcnt (uint64 x)

{
  int64 ans;
  asm ("popcnt %1,%0" : "=r" (ans) : "r" (x));
  return ans;
}

static INLINE int64 _poppar (uint64 x)

{
  int64 ans;
  asm ("popcnt %1,%0" : "=r" (ans) : "r" (x));
  return ans & 1;
}

static INLINE int64 _trailz (uint64 x)

{
  return _popcnt ((~x) & (x-1));
}

#else  // x86 without popcnt

static INLINE int64 _popcnt (uint64 x)

{
  x = x - ((x >> 1) & _MASK1BIT);
  x = ((x >> 2) & _MASK2BITS) + (x & _MASK2BITS);
  x = ((x >> 4) + x) & _MASK4BITS;
  // preferred on machines with fast 64-bit multiply 
  x = x * UI64(0x0101010101010101);
  return x >> 56;
}

static INLINE int64 _poppar (uint64 x)

{
  x ^= x >> 32;
  x ^= x >> 16;
  x ^= x >>  8;
  x ^= x >>  4;
  // 4-bit lookup of poppar vals 
  x  = 0x6996 >> (x & 0xf);
  return x&1;
}

// enabled as of April 2007
// Nov '04: microcode on Opteron so not that great...
// Apr '07: very good on Intel Core 2
static INLINE int64 _trailz (uint64 x)

{
  int64 ans;
  const int64 tmp=64;

  asm ("bsf %1,%0 ; cmovz %2,%0" : "=r" (ans) : "r" (x), "r" (tmp));
  return ans;
}

#endif  // End: popcnt / no popcnt

#if defined(BITOPS_HW_LEADZ) && (BITOPS_HW_LEADZ > 0)  // x86 with lzcnt

static INLINE int64 _leadz (uint64 x)

{
  int64 ans;

  // Nov '09: correct and fast on modern AMD, but not Intel as yet
  // Note: lzcnt uses the same opcode as bsr, but different prefix.
  //       So older chips will actually give incorrect results!
  asm ("lzcnt %1,%0" : "=r" (ans) : "r" (x));
  return ans;
}

#else  // x86 without lzcnt

// enabled as of April 2007
// Nov '04: microcode on Opteron so not that great...
// Apr '07: very good on Intel Core 2
static INLINE int64 _leadz (uint64 x)

{
  int64 ans;
  const int64 tmp=-1;

  // use BSR
  asm ("bsr %1,%0 ; cmovz %2,%0" : "=r" (ans) : "r" (x), "r" (tmp));
  return 63-ans;
}

#endif // End: lzcnt / no lzcnt

#else // chips without hardware support for special instructions 
      // also includes x86 for compilers other than GCC / ICC

// workarounds for MIPSpro Compilers: Version 7.30 
#if defined(__sgi) && defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 730)
#define _leadz  _leadzsgi
#define _poppar _popparsgi
#define _popcnt _popcntsgi
#endif 

static INLINE int64 _popcnt (uint64 x)

{
  x = x - ((x >> 1) & _MASK1BIT);
  x = ((x >> 2) & _MASK2BITS) + (x & _MASK2BITS);
  x = ((x >> 4) + x) & _MASK4BITS;
#if defined(__x86_64) || defined(__x86_64__)
  // preferred on machines with fast 64-bit multiply 
  x = x * UI64(0x0101010101010101);
  return x >> 56;
#else
  x = (x >>  8) + x;
  x = (x >> 16) + x;
  x = (x >> 32) + x;
  return x & 0xff;
#endif
}

static INLINE int64 _poppar (uint64 x)

{
  x ^= x >> 32;
  x ^= x >> 16;
  x ^= x >>  8;
  x ^= x >>  4;
  // 4-bit lookup of poppar vals 
  x  = 0x6996 >> (x & 0xf);
  return x&1;
}

static INLINE int64 _leadz (uint64 x)

{
  // propogate 1's 
  x |= x >>  1;
  x |= x >>  2;
  x |= x >>  4;
  x |= x >>  8;
  x |= x >> 16;
  x |= x >> 32;

  // popcnt(~x) == # of leading zeroes 
  x = ~x - ((~x >> 1) & _MASK1BIT);
  x = ((x >>  2) & _MASK2BITS) + (x & _MASK2BITS);
  x = ((x >>  4) + x) & _MASK4BITS;
#if defined(__x86_64) || defined(__x86_64__)
  // preferred on machines with fast 64-bit multiply 
  x = x * UI64(0x0101010101010101);
  return x >> 56;
#else
  x = (x >>  8) + x;
  x = (x >> 16) + x;
  x = (x >> 32) + x;
  return x & 0xff;
#endif
}

static INLINE int64 _trailz (uint64 x)

{
  x = (~x) & (x-1);

  // popcnt(x) == # of trailing zeroes 
  x = x - ((x >> 1) & _MASK1BIT);
  x = ((x >>  2) & _MASK2BITS) + (x & _MASK2BITS);
  x = ((x >>  4) + x) & _MASK4BITS;
#if defined(__x86_64) || defined(__x86_64__)
  // preferred on machines with fast 64-bit multiply 
  x = x * UI64(0x0101010101010101);
  return x >> 56;
#else
  x = (x >>  8) + x;
  x = (x >> 16) + x;
  x = (x >> 32) + x;
  return x & 0xff;
#endif
}

#endif // End: chips without hardware support for special instructions 

#else // defined _CRAYC 

#include <intrinsics.h>

static INLINE int64 _trailz (uint64 x)

{
  return _popcnt ((~x) & (x-1));
}

#pragma _CRI inline _trailz

#endif

//
//  End: SECTION 2


//                       SECTION 3:                         
//                                                          
//             Architecture-dependent features              
//                                                          

//                       SECTION 3a:                        
//                                                          
//            Alpha-based systems: DEC, T3D, T3E            
//                                                          

#ifdef __alpha 

#ifdef __DECC

////                   multiply high support              

#include <machine/builtins.h>
// newer DEC compilers support this directly 
#ifndef _int_mult_upper
#define _int_mult_upper      __UMULH

// ia64 intrinsics xmpy.h xmpy.hu 
#endif

////                 byte compare/mask support            

#include <c_asm.h>
#define _byte_comp(a,b)      asm ("cmpbge %a0, %a1, %v0", a, b)
#define _byte_kill(w,b)      asm ("zap    %a0, %a1, %v0", w, b)
#define _byte_keep(w,b)      asm ("zapnot %a0, %a1, %v0", w, b)

////                 ev6 multimedia instructions          

// note: PERR is broken in 4.0D and earlier 
#define _byte_diff(a,b)      asm ("perr   %a0, %a1, %v0", a, b)
#define _byte_sum(a)         _byte_diff(a,0)
#define _byte_umin(a,b)      asm ("minub8 %a0, %a1, %v0", a, b)
#define _byte_smin(a,b)      asm ("minsb8 %a0, %a1, %v0", a, b)
#define _byte_umax(a,b)      asm ("maxub8 %a0, %a1, %v0", a, b)
#define _byte_smax(a,b)      asm ("maxsb8 %a0, %a1, %v0", a, b)
#define _word_umin(a,b)      asm ("minuw4 %a0, %a1, %v0", a, b)
#define _word_smin(a,b)      asm ("minsw4 %a0, %a1, %v0", a, b)
#define _word_umax(a,b)      asm ("maxuw4 %a0, %a1, %v0", a, b)
#define _word_smax(a,b)      asm ("maxsw4 %a0, %a1, %v0", a, b)

////               high-resolution clock support          *

#define _rtc_raw()           __RPCC()

// alternate _rtc using DEC asm 
//#include <c_asm.h>
//#define _rtc_raw()       asm ("rpcc %v0")

static INLINE int64 _rtc_correct (uint64 x)

{
  return (x + (x >> 32)) & 0xffffffffuL;
}

#define  _rtc() _rtc_correct(_rtc_raw())

#endif // __DECC 

#ifdef __GNUC__

////                   multiply high support              

#define _int_mult_upper(x,y) ({uint64 out;\
  asm("umulh %1,%2,%0" : "=r" (out) : "r" (x), "r" (y)); out;})

////                 byte compare/mask support            

#define _byte_comp(a,b)  ({uint64 ans;\
  asm ("cmpbge %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})

#define _byte_kill(w,b)  ({uint64 ans;\
  asm ("zap    %1,%2,%0" : "=r" (ans) : "r" (w), "r" (b)); ans;})

#define _byte_keep(w,b)  ({uint64 ans;\
  asm ("zapnot %1,%2,%0" : "=r" (ans) : "r" (w), "r" (b)); ans;})

////                 ev6 multimedia instructions          

// requires -mmax or -mcpu=ev6 flags 
#define _byte_diff(a,b)  ({uint64 ans;\
  asm ("perr   %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _byte_sum(a)     _byte_diff(a,0)
#define _byte_umin(a,b)  ({uint64 ans;\
  asm ("minub8 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _byte_smin(a,b)    ({uint64 ans;\
  asm ("minsb8 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _byte_umax(a,b)    ({uint64 ans;\
  asm ("maxub8 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _byte_smax(a,b)    ({uint64 ans;\
  asm ("maxsb8 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _word_umin(a,b)    ({uint64 ans;\
  asm ("minuw4 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _word_smin(a,b)    ({uint64 ans;\
  asm ("minsw4 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _word_umax(a,b)    ({uint64 ans;\
  asm ("maxuw4 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})
#define _word_smax(a,b)    ({uint64 ans;\
  asm ("maxsw4 %1,%2,%0" : "=r" (ans) : "r" (a), "r" (b)); ans;})

////               high-resolution clock support          

#define _rtc_raw() ({int64 tmp;\
  asm volatile ("rpcc %0": "=r" (tmp) :); tmp;})

#ifdef _CRAYMPP
#define _rtc_correct(t_in) ({\
  int64 __t = (t_in);\
  int64 __inc = 1L<<32;\
  int64 __tmp2;\
  if (__t >= 0L) __inc = 0L;\
  __tmp2 = __t << 32;\
  if (__tmp2 < 0L) __inc=0L;\
  __t += __inc;\
  __t &=  0XFFFFFFFFFFFFFFL;\
})

#else // ndef _CRAYMPP 
#define _rtc_correct(A) ({\
  uint64 x = (A);\
  (x + (x >> 32)) & 0xffffffffuL;\
})

#endif // def/ndef _CRAYMPP 

#define  _rtc() _rtc_correct(_rtc_raw())

#endif // __GNUC__ 

#endif // __alpha 

//                       SECTION 3b:                          
//                                                            
//                 Itanium-based systems                      
//                                                            

#if defined(__ia64) || defined(__ia64__)

#if   defined(__INTEL_COMPILER)

#define _int_mult_upper(A,B)  _m64_xmahu(A,B,0)

static INLINE int64 _byte_diff (uint64 x, uint64 y)
{
  return _m_to_int64(_mm_sad_pu8(_m_from_int64(x),_m_from_int64(y)));
}

#define _rtc _rtc_fcn
static INLINE uint64 _rtc_fcn (void)
  
{
  // #pragma intrinsic(__getReg) -- no longer needed (sep '08)
  // rdtsc() equivalent for Itanium
  //  return __getReg(3116);
  return __getReg(_IA64_REG_AR_ITC);
}

#elif defined(__hpux)

#define _int_mult_upper _int_mult_upper_fcn
static INLINE uint64 _int_mult_upper_fcn (uint64 i1, uint64 i2)
{
  double xi1, xi2, x;

  xi1 = _Asm_setf(_FR_SIG,i1);
  xi2 = _Asm_setf(_FR_SIG,i2);
  x   = _Asm_xma(_XM_HU,xi1,xi2,(double)0.);
  return _Asm_getf(_FR_SIG,x);
}

static INLINE int64 _byte_diff (uint64 x, uint64 y)
{
  return _Asm_psad1(x,y);
}

#elif defined(__GNUC__)

#define _int_mult_upper _int_mult_upper_fcn
static INLINE uint64 _int_mult_upper_fcn (uint64 x, uint64 y)

{
  uint64 ans;
  double i1, i2, pr;
  
  asm ("setf.sig %0=%1"    : "=f" (i1)  : "r" (x));
  asm ("setf.sig %0=%1"    : "=f" (i2)  : "r" (y));
  asm ("xmpy.hu  %0=%1,%2" : "=f" (pr)  : "f" (i1), "f" (i2));
  asm ("getf.sig %0=%1"    : "=r" (ans) : "f" (pr));
  return ans;  
}

static INLINE int64 _byte_diff (uint64 x, uint64 y)

{
  int64 ans;

  asm ("psad1 %0=%1,%2" : "=r" (ans) : "r" (x), "r" (y));
  return ans;
}

#define _rtc _rtc_fcn
static INLINE uint64 _rtc_fcn (void)

{
  uint64 rtime;

  // application register 44 is the cycle-accurate timer
  asm ("mov.m %0=ar44" : "=r" (rtime));
  return rtime;
}

#endif // End: which IA64 compiler   

#define _byte_sum(a)     _byte_diff(a,0)

#endif // ia64   

//                       SECTION 3c:                          
//                                                            
//             Opteron(x86-64)-based systems                  
//                                                            

#if defined(__x86_64) || defined(__x86_64__)

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__PGI)

////                   multiply high support              

#define _int_mult_upper _int_mult_upper_fcn
static INLINE uint64 _int_mult_upper_fcn (uint64 x, uint64 y)

{
  uint64 ans;

#ifdef __PGI  // Why doesn't the more concise asm() work??
  asm ("mov %1, %%rax ; mul %2 ; mov %%rdx,%0" :  "=r" (ans) : "r" (x), "r" (y) : "rax", "rdx");
#else
  // better asm
  asm ("mul %2" : "=d" (ans), "+a" (x) : "r" (y));
#endif
  return ans;
}

#define _rtc _rtc_fcn
static INLINE uint64 _rtc_fcn (void)

{
  uint64 ans;
  uint32 hi=0, lo=0;
    
  asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
  ans = hi;
  ans = (ans << 32) | lo;
  return ans;
}

#endif // End: GNUC/INTEL_C   

#endif // x86-64   

//                       SECTION 3d:                          
//                                                            
//                Power and PowerPC systems                   
//                                                            

#if defined(__PPC__) || defined(__POWERPC__) || defined(__ppc__) || defined(__powerpc__)

#if defined(__IBMC__) || defined(__IBMCPP__)

#define _int_mult_upper __mulhdu

// Note (Mar '11): on Power 7 mftb seems to run at 512 MHz

#define _rtc _rtc_fcn
static INLINE uint64 _rtc_fcn (void)
  
{
  uint64 ret;
  
  __fence();
  ret = __mftb();
  __fence();

  return ret;
}

#elif defined(__GNUC__)

#define _int_mult_upper _int_mult_upper_fcn
static INLINE uint64 _int_mult_upper_fcn (uint64 x, uint64 y)

{
  uint64 ans;

  asm ("mulhdu %0,%1,%2" : "=r" (ans) : "r" (x) , "r" (y));
  return ans;
}

#define _rtc _rtc_fcn
static INLINE uint64 _rtc_fcn (void)
  
{
  uint64 ret;

  asm volatile ("mftb %0" : "=r" (ret));

  return ret;
}

#endif // End: which Power compiler   

#endif // Power, PowerPC   

//                       SECTION 3f:                          
//                                                            
//                      Mips/SiCortex                           
//                                                            

////                   multiply high support              
#if defined (__mips64)

#if (defined(__PATHCC__) || defined(__GNUC__))
#define _int_mult_upper _int_mult_upper_fcn
static INLINE uint64 _int_mult_upper_fcn (uint64 x, uint64 y)

{
  uint64 ans;

  asm("dmultu %1,%2; mfhi %0" : "=r" (ans) : "r" (x), "r" (y));
  return ans;
}
#endif // End: pathcc/gcc
  
#endif // End: Mips


//
//  End: SECTION 3


//                       SECTION 4:                           
//                                                            
//           fast 64-bit random number generator              
//             (reasonably free of properties)                

#define _BR_RUNUP_      128
#define _BR_LG_TABSZ_     7
#define _BR_TABSZ_      (I64(1)<<_BR_LG_TABSZ_)

typedef struct {
  uint64 hi, lo, ind;
  uint64 tab[_BR_TABSZ_];
} brand_t;

#define _BR_64STEP_(H,L,A,B) {\
  uint64 x;\
  x = H ^ (H << A) ^ (L >> (64-A));\
  H = L | (x >> (B-64));\
  L = x << (128 - B);\
}

static INLINE uint64 brand (brand_t *p) {
  uint64 hi=p->hi, lo=p->lo, i=p->ind, ret;

  ret = p->tab[i];

  // 64-step a primitive trinomial LRS:  0, 45, 118   
  _BR_64STEP_(hi,lo,45,118);

  p->tab[i] = ret + hi;

  p->hi  = hi;
  p->lo  = lo;
  p->ind = hi & _maskr(_BR_LG_TABSZ_);

  return ret;
}

#ifdef _CRAYC
#pragma _CRI inline brand
#endif

#if ((! defined(_CRAY)) || defined(_CRAYIEEE))

static INLINE double dbrand (brand_t *p)

{
  uint64 x;

#if 1
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  const double n = 0x1.0p-52;  // C99 hexadecimal floating-point
#else
  const double n = 2.2204460492503130808e-16;  // decimal floating-point equivalent
#endif
  // new preferred version
  x = brand(p) & _maskr(52);
  return (double)x * n;
#elif 1
  // !!! only works for IEEE !!!   
  union {uint64 x; double d;} u;
  u.x = brand(p) & _maskr(52);
  u.x |= I64(1023) << 52;
  return u.d-1.;
#else
  // !!! only works for IEEE !!!   
  // original version, gcc 4.1 had problems
  double *d;
  x  = brand(p) & _maskr(52);
  x |= I64(1023) << 52;
  d  = (double *) (void *) &x;  // going thru (void *) eliminates some warnings
  return *d-1.;
#endif
}

#endif

#ifdef __GNUC__
static void brand_init (brand_t *p, uint64 val)  __attribute__ ((unused));  // eliminates warning if unused
#endif

static void brand_init (brand_t *p, uint64 val)

{
  int64 i;
  uint64 hi, lo;

  hi = UI64(0x9ccae22ed2c6e578) ^ val;
  lo = UI64(0xce4db5d70739bd22) & _maskl(118-64);

  // we 64-step 0, 33, 118 in the initialization   
  for (i = 0; i < 64; i++)
    _BR_64STEP_(hi,lo,33,118);
  
  for (i = 0; i < _BR_TABSZ_; i++) {
    _BR_64STEP_(hi,lo,33,118);
    p->tab[i] = hi;
  }
  p->ind = _BR_TABSZ_/2;
  p->hi  = hi;
  p->lo  = lo;

  for (i = 0; i < _BR_RUNUP_; i++)
    brand(p);
}

//
//  End: SECTION 4


//                       SECTION 5:                           
//                                                            
//       portable set of prefetching macros
//  
// Syntax is _prefetch[_SUFFIX] where SUFFIX can be any
// combination of f, n, x in that order.
//   f -> allow page faults
//   n -> non-temporal
//   x -> exclusive/modify intent
//
// Not all machines support all variants, so we will perform replacements
// as necessary, e.g. _pretetch_fx -> _prefetch_x.
//
// Care must be taken with 'f' in particular since prefetching off
// the end of any array in this case could cause SEGV.
//
// Note: most evidence to date suggests exclusive has no effect,
//   or a negative effect on codes.  We have it here because most
//   machines support it, but also to allow for easier experimentation
//   with this type of prefetch.

//
// Alpha/Tru64/Dec C
//
#ifdef __DECC
#define _prefetch(a)     asm  ("ldl %r31, 0(%a0)", (void *)(a))
#define _prefetch_n(a)   asm  ("ldq %r31, 0(%a0)", (void *)(a))
#define _prefetch_x(a)   dasm ("lds %f31, 0(%a0)", (void *)(a))

//
// Itanium/Intel C
//
#elif (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
// prefetch type:    allocated where:
// _lfhint_none       L1, L2, L3
// _lfhint_nt1        L2, L3
// _lfhint_nt2        L2, L3  [L2 LRU not updated]
// _lfhint_nta        L2      [L2 LRU not updated]
#define _prefetch(a)     __lfetch           (__lfhint_none, (void *)(a))
#define _prefetch_f(a)   __lfetch_fault     (__lfhint_none, (void *)(a))
#define _prefetch_n(a)   __lfetch           (__lfhint_nt2,  (void *)(a))
#define _prefetch_fn(a)  __lfetch_fault     (__lfhint_nt2,  (void *)(a))
#define _prefetch_x(a)   __lfetch_excl      (__lfhint_none, (void *)(a))
#define _prefetch_fx(a)  __lfetch_fault_excl(__lfhint_none, (void *)(a))
#define _prefetch_nx(a)  __lfetch_excl      (__lfhint_nt2,  (void *)(a))
#define _prefetch_fnx(a) __lfetch_fault_excl(__lfhint_nt2,  (void *)(a))

//
// Itanium/HP C
// (Note: doesn't seem to support 'exclusive')
//
#elif (defined(__ia64) || defined(__ia64__)) && defined(__hpux)
#define _prefetch(a)     _Asm_lfetch(_LFTYPE_NONE,  _LFHINT_NONE, (void *)(a))
#define _prefetch_f(a)   _Asm_lfetch(_LFTYPE_FAULT, _LFHINT_NONE, (void *)(a))
#define _prefetch_n(a)   _Asm_lfetch(_LFTYPE_NONE,  _LFHINT_NT2,  (void *)(a))
#define _prefetch_fn(a)  _Asm_lfetch(_LFTYPE_FAULT, _LFHINT_NT2,  (void *)(a))

//
// PA-RISC/HP-UX
//
#elif defined(__hppa)
#define _prefetch(a)     {\
  register uint64 the_addr=(uint64)(a);\
  _asm("LDW",0,0,(the_addr),0);\
}

//
// x86-64/GCC,Intel C
//
#elif (defined(__x86_64) || defined(__x86_64__)) &&(defined(__GNUC__) || defined(__INTEL_COMPILER))
#define _prefetch(a)							\
  __asm__ __volatile__ ("prefetcht0 (%0)"  :: "r" ((void *)(a)))
#define _prefetch_n(a)							\
  __asm__ __volatile__ ("prefetchnta (%0)" :: "r" ((void *)(a)))  
// note: prefetchw is not available on all x86-64 CPUs
#if 0
#define _prefetch_x(a)							\
  __asm__ __volatile__ ("prefetchw (%0)"   :: "r" ((void *)(a)))
#endif

// Power, PowerPC

#elif defined(__PPC__) || defined(__POWERPC__) || defined(__ppc__) || defined(__powerpc__)

#if defined(__IBMC__) || defined(__IBMCPP__)

#define _prefetch   __dcbt
#define _prefetch_x __dcbtst

#if defined(__IBM_GCC_ASM) && (__IBM_GCC_ASM > 99)  // Disabled: As of Mar '11 this breaks xlc/as badly

#define _prefetch_n(a)							\
  __asm__ __volatile__ ("dcbt 0,%0,16"    :: "r" ((void *)(a)))
#define _prefetch_nx(a)							\
  __asm__ __volatile__ ("dcbtst 0,%0,16"  :: "r" ((void *)(a)))

#endif  // End: xlc w/ GNU asm

#elif defined(__GNUC__)

// Notes on last argument to prefetches:
//   ,0  -> into L1
//   ,2  -> into L2
//   ,16 -> transient
//   plus other ones related to streams

#define _prefetch(a)							\
  __asm__ __volatile__ ("dcbt 0,%0,0"     :: "r" ((void *)(a)))
#define _prefetch_n(a)							\
  __asm__ __volatile__ ("dcbt 0,%0,16"    :: "r" ((void *)(a)))
#define _prefetch_x(a)							\
  __asm__ __volatile__ ("dcbtst 0,%0,0"   :: "r" ((void *)(a)))
#define _prefetch_nx(a)							\
  __asm__ __volatile__ ("dcbtst 0,%0,16"  :: "r" ((void *)(a)))

#endif // End: which Power compiler

#endif  // End: Arch/Compiler combinations

//
// Fill in gaps for missing variants.  I consider "fault"
// to be the most esoteric variant, so drop it first.
//
#ifndef _prefetch
#define _prefetch
#endif

#ifndef _prefetch_f
#define _prefetch_f _prefetch
#endif

#ifndef _prefetch_n
#define _prefetch_n _prefetch
#endif

#ifndef _prefetch_x
#define _prefetch_x _prefetch
#endif

#ifndef _prefetch_fn
#define _prefetch_fn _prefetch_n
#endif

#ifndef _prefetch_fx
#define _prefetch_fx _prefetch_x
#endif

#ifndef _prefetch_nx
#define _prefetch_nx _prefetch_n
#endif

#ifndef _prefetch_fnx
#define _prefetch_fnx _prefetch_nx
#endif

//
//  End: SECTION 5

//                       SECTION 6:                           
//
// routines for accelerating division with fixed divisor
// requires _int_mult_upper
// currently we only allow for 63-bit numerators but this
// restriction may be lifted in the future
//
// Usage:
//   first call _divbymul_prep() to precompute for division by d
//   then call _divbymul63 to quickly compute x/d where x <= 2^63

#ifdef _int_mult_upper  // requires multiply high

typedef struct {
  int64 shift;
  uint64 magic;
} divbymul_t;

static INLINE
uint64 _divbymul63       (uint64 numerator, divbymul_t div)

{
  // note: only works if numerator <= 2^63
  return _int_mult_upper (numerator, div.magic) >> div.shift;
}

#ifdef __GNUC__
// eliminates warning if unused
static int64 _divbymul_prep (uint64 divisor, divbymul_t *div)
  __attribute__ ((unused));
#endif

static 
int64  _divbymul_prep    (uint64 divisor, divbymul_t *div)

{
  int64 k, shift;
  uint64 magic;
  
  if (divisor <= 1)  return 1;  // invalid
  
  shift = 63 - _leadz( divisor-1 );
  magic = (~UI64(0)) / divisor;
  for (k = 1; k <= shift; k++) {
    magic <<= 1;
    magic += _int_mult_upper( magic+1, divisor ) < (UI64(1) << k);
  }
  magic += _int_mult_upper( magic, divisor ) < (UI64(1) << k);

  div->magic = magic;
  div->shift = shift;
  return 0;
}

#endif // def _int_mult_upper

//
//  End: SECTION 6

#endif // ndef BITOPS_H   


