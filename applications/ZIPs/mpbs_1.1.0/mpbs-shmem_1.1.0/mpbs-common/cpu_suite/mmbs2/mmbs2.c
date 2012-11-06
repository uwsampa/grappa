/**
 * SSE2 intrinsics are not supported in craycc.  If we're using the Cray
 * compiler, just insert a stub function instead.
 */

#ifndef _CRAYC

#include <bench.h>
#include "mmbs2.h"

void mmbs2(vec_t x[VLEN], vec_t y[VLEN]) {
  vec_t c[NWORDS] __attribute__ ((aligned (ALIGN)));
  vec_t tempy, temp1, temp2;
  int i, j, ii, k;

  /* initialize the sum to: x, if its low bit is 1, 0 otherwise. */
  for (i = 0; i < N; i++)
    for (j = 0; j < NWORDS; j++)
      y[NWORDS*i + j] = vec_and(x[NWORDS*i + j], x[j]);

  /* initialize the carry to 0 */
  for (i = 0; i < NWORDS; i++)
    c[i] = vec_xor(x[0], x[0]);

  /* add cls(x,k) + carry to y, but use 0 if x<1,k> = 0 */
  for (k = 1; k < N; k++) {
    ii = N - k; /* will always be (i + N - k) % N */

    /* loop on bits. low bit is bit0 */
    for (i = 0; i < N; i++) {
      for (j = 0; j < NWORDS; j++) {
        /* we add x <<< k only if the k-th bit of x is not 0 */
        tempy = y[NWORDS*i + j];
        temp1 = vec_xor(tempy, vec_and(x[NWORDS*ii + j], x[NWORDS*k + j]));
        temp2 = vec_xor(tempy, c[j]);

        /* new sum bit is y^x^c */
        y[NWORDS*i + j] = vec_xor(temp1, c[j]);

        /* new carry bit is y ^ ((y^x) & (y^c)) */
        c[j] = vec_xor(tempy, vec_and(temp1, temp2));
      }

      ii = (ii + 1) % N;
    }

    /* add in final carry */
    for (i = 0; i < N; i++) {
      for (j = 0; j < NWORDS; j++) {
        tempy = y[NWORDS*i + j];

        /* new sum bit is y ^ c */
        y[NWORDS*i + j] = vec_xor(tempy, c[j]);

        /* new carry bit is y & c */
        c[j] = vec_and(tempy, c[j]);
      }
    }
  }
}

#else
void mmbs2(void *x, void *y) {
}
#endif
