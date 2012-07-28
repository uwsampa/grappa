/* CVS info                         */
/* $Date: 2010/04/02 20:46:03 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: xba.c,v $              */
/* $State: Stab $:                */

#include "xba.h"

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:46:03 $ $Revision: 1.5 $ $RCSfile: xba.c,v $ $Name:  $";

void xor_bit_arr (uint64 * RESTRICT mat, brand_t *br,
		  uint64 * RESTRICT ans, int64 n)

{
  int64 i;
  uint64 vec1, sum1;
  uint64 vec2, sum2;

  while (n > 0) {
    sum1 = 0;
    vec1 = brand(br);
    sum2 = 0;
    vec2 = brand(br);

// can insert #pragma unroll directive here, with any desired parms
// if compiler supports a #pragma unroll with loop scope
    for (i = 0; i < NROWS; i++) {
      uint64 tmp = mat[i]; // always safe to load
      sum1 ^= (vec1 & 1) ? tmp : 0;
      vec1 >>= 1;
      if (vec2&1) sum2 ^= tmp;
        vec2 >>= 1;
    }
    *ans++ ^= sum1;  n--;
    *ans++ ^= sum2;  n--;
  }
}
