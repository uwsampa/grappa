#ifndef MMBS2_H
#define MMBS2_H

#include "mm_ops.h"

/* tunable */

#define PAD      32L
#define NWORDS   4L               /* vector length for bit-serial computation
				   * (must divide TOT below)
				   */
#define ALIGN    64               /* __attribute__ ((aligned (ALIGN)))
                                     used for __m128is
                                     must be a multiple of 16 */

/* end tunable */

#define N        16L              /* size of our bit-serial "word" (bsw) */
#define VLEN     (N * NWORDS)     /* size of our short vector of bsw's   */
#define TOT      32L              /* total bsw's in memory at one time   */
#define TLEN     (N * TOT)        /* size of total bsw's                 */

void mmbs2(vec_t x[VLEN], vec_t y[VLEN]);
void mmtilt(vec_t x[64]);

#endif
