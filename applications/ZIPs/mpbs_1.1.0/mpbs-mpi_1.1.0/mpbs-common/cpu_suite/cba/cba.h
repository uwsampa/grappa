#include "bench.h"

/*********************************\
* CVS info                        *
* $Date: 2010/04/02 20:37:24 $    *
* $Revision: 1.5 $                *
* $RCSfile: cba.h,v $             *
* $State: Stab $:               *
\*********************************/

/* can tune BLOCKSIZE, NITERS, PAD
 * can #define RESTRICT to appropriate symbol if
 *   restricted pointers are supported
 * see "TUNING" document for information on USE_POP3
 */


#define BLOCKSIZE    32L            /* MUST be a power of 2
				     * and <= 64 due to our test params */
#define NITERS       60*64L         /* should be a multiple of 64 */
#define PAD          16L

/* restricted pointers */
#ifndef RESTRICT
/* if no support */
#define RESTRICT
#endif

#define M1 0x5555555555555555uL
#define M2 0x3333333333333333uL
#define M3 0x0f0f0f0f0f0f0f0fuL

#ifdef USE_POP3

#define POPCNT3(A,B,C,Z) {\
  uint64 X,Y;\
  X = ( (A)       & M1) + ( (B)       & M1) + ( (C)       & M1);\
  Y = (((A) >> 1) & M1) + (((B) >> 1) & M1) + (((C) >> 1) & M1);\
  Z = (X & M2) + ((X >> 2) & M2) + \
      (Y & M2) + ((Y >> 2) & M2);\
  Z = (Z & M3) + ((Z >> 4) & M3);\
  Z = Z + (Z >> 8);\
  Z = Z + (Z >> 16);\
  Z = Z + (Z >> 32);\
  Z&= 0xff;\
}

#define _MASK1BIT  0x5555555555555555uL
#define _MASK2BITS 0x3333333333333333uL
#define _MASK4BITS 0x0f0f0f0f0f0f0f0fuL

static int64 _popcnt (uint64 x)

{
  x = x - ((x >> 1) & _MASK1BIT);
  x = ((x >> 2) & _MASK2BITS) + (x & _MASK2BITS);
  x = ((x >> 4) + x) & _MASK4BITS;
  x = (x >>  8) + x;
  x = (x >> 16) + x;
  x = (x >> 32) + x;

  return x & 0xff;
}

#else /* undef USE_POP3 */

/* #define your _popcnt() macro here */

#if defined (__INTEL_COMPILER)
#include <nmmintrin.h>
#define _popcnt(x) _mm_popcnt_u64(x);
#elif defined(_CRAYC)
#include <intrinsics.h>
#elif defined(__PGI) // doesn't compile
#include <intrin.h>
#elif defined(__GNUC__)	//pathscale as well
#define _popcnt(x) __builtin_popcount(x);
#endif 


#endif




int64 cnt_bit_arr    (uint64 *arr, int64 nrow, int64 ncol, uint64 *out,
		      int64 niters);
int64 cnt_bit_arr_nb (uint64 *arr, int64 nrow, int64 ncol, uint64 *out,
		      int64 niters);
void  blockit        (uint64 *data, int64 nrow, int64 ncol, uint64 *work);
