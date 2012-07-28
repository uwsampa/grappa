#ifndef __MMBS2_H
#define __MMBS2_H

/**
 * This header is a subset of mm_ops.h from CPU suite and mmbs2.h from the
 * original kernel and is only included if the SSE2 instruction set is
 * supported.
 *
 * This kernel is NOT supported if compiling with craycc
 */

#ifndef _CRAYC

#if defined(__SSE2__)
#include <emmintrin.h>

#include "types.h"

typedef __m128i vec_t;
typedef union {
  struct {
    uint64 u[2];
  }as64;
  vec_t m;
  struct {
    uint32 w[4];
  }as32;
} um128;

static inline vec_t vec_set64_1(uint64 val) {
  vec_t x;
  ((um128 *)&x)->as64.u[0] = val;
  ((um128 *)&x)->as64.u[1] = val;
  return x;
}
#define vec_xor(x,y) _mm_xor_si128((x),(y))

/* tunable */

#define PAD      32L
#define NWORDS   4L               /* vector length for bit-serial computation
				   * (must divide TOT below)
				   */
#define ALIGN    64               /* __attribute__ ((aligned (ALIGN)))
                                     used for __m128is
                                     must be a multiple of 16 */

/* end tunable */

#define VEC_WIDTH 128
#define N        16L              /* size of our bit-serial "word" (bsw) */
#define VLEN     (N * NWORDS)     /* size of our short vector of bsw's   */
#define TOT      32L              /* total bsw's in memory at one time   */
#define TLEN     (N * TOT)        /* size of total bsw's                 */

void mmbs2(vec_t x[VLEN], vec_t y[VLEN]);
#endif /* SSE2 */

#else
void mmbs2(void *x, void *y);
#endif /* _CRAYC */

#endif
