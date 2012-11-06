/* CVS info                      */
/* $Date: 2010/04/02 20:37:10 $  */
/* $Revision: 1.5 $              */
/* $RCSfile: cba.c,v $           */
/* $State: Stab $              */

#include "common_inc.h"
#include "error_routines.h"
#include "bm_timers.h"

#include "cba.h"

/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:37:10 $ $Revision: 1.5 $ $RCSfile: cba.c,v $ $Name:  $";

static
void  block_iter     (uint64 * RESTRICT accum, uint64 * RESTRICT arr,
		      uint64 * RESTRICT out, int64 niters)

{
  int64 i, j;
  uint64 ind0, ind1, ind2, ind3, s0, s1, s2, s3;

  for (i = 0; i < niters; i+=4) {
    s0 = out[i+0];  ind0 = s0 >> 48;
    s1 = out[i+1];  ind1 = s1 >> 48;
    s2 = out[i+2];  ind2 = s2 >> 48;
    s3 = out[i+3];  ind3 = s3 >> 48;

    j = 0;
#ifdef USE_POP3
    for (; j < (BLOCKSIZE-2); j+=3) {
      uint64 x, y, z, popc;

      x = accum[j+0];  y = accum[j+1];  z = accum[j+2];

      x ^= arr[ind0*BLOCKSIZE + j+0];
      y ^= arr[ind0*BLOCKSIZE + j+1];
      z ^= arr[ind0*BLOCKSIZE + j+2];
      POPCNT3(x,y,z,popc);  s0 += popc;

      x ^= arr[ind1*BLOCKSIZE + j+0];
      y ^= arr[ind1*BLOCKSIZE + j+1];
      z ^= arr[ind1*BLOCKSIZE + j+2];
      POPCNT3(x,y,z,popc);  s1 += popc;

      x ^= arr[ind2*BLOCKSIZE + j+0];
      y ^= arr[ind2*BLOCKSIZE + j+1];
      z ^= arr[ind2*BLOCKSIZE + j+2];
      POPCNT3(x,y,z,popc);  s2 += popc;

      x ^= arr[ind3*BLOCKSIZE + j+0];
      y ^= arr[ind3*BLOCKSIZE + j+1];
      z ^= arr[ind3*BLOCKSIZE + j+2];
      POPCNT3(x,y,z,popc);  s3 += popc;

      accum[j+0] = x;  accum[j+1] = y;  accum[j+2] = z;
    }
#endif
    for (; j < BLOCKSIZE; j++) {
      uint64 x = accum[j];

      x ^= arr[ind0*BLOCKSIZE + j];  s0 += _popcnt(x);
      x ^= arr[ind1*BLOCKSIZE + j];  s1 += _popcnt(x);
      x ^= arr[ind2*BLOCKSIZE + j];  s2 += _popcnt(x);
      x ^= arr[ind3*BLOCKSIZE + j];  s3 += _popcnt(x);
      accum[j] = x;
    }
    out[i+0] = s0;
    out[i+1] = s1;
    out[i+2] = s2;
    out[i+3] = s3;
  }
}

int64 cnt_bit_arr    (uint64 *arr, int64 nrow, int64 ncol, uint64 *out,
		      int64 niters)

{
  int64 i;

  /* call the rows of the matrix r(0) ... r(ncol-1)
   * let the random row indices (stored in out[])
   *    be i(0) ... i(NITERS-1) [note that each index is > 0]
   * we will accumulate each random row into r(0)
   *    and return the number of ones in the result
   */

  for (i = 0; i < ncol; i+=BLOCKSIZE)
    block_iter (&arr[i*nrow], &arr[i*nrow], out, niters);

  for (i = 0; i < niters; i++)
    out[i] &= _maskr(48);

  return 0;
}

/* non-blocked code */

int64 cnt_bit_arr_nb (uint64 *arr, int64 nrow, int64 ncol, uint64 *out,
		      int64 niters)

{
  int64 i, j;

  for (i = 0; i < niters; i++) {
    uint64 ind, s;

    s = out[i];  ind = s >> 48;

    for (j = 0; j < ncol; j++) {
      uint64 x = arr[j];

      x ^= arr[ind*ncol + j];
      s += _popcnt(x);


      arr[j] = x;
    }
    out[i] = s;
  }

  for (i = 0; i < niters; i++)
    out[i] &= _maskr(48);

  return 0;
}

/* convert data into blocked format */

void  blockit        (uint64 *data, int64 nrow, int64 ncol, uint64 *work)

{
  int64 b, i, j;

  /* loop over blocks */
  for (b = 0; b < (ncol/BLOCKSIZE); b++) {
    /* loop over rows in block */
    for (i = 0; i < nrow; i++)
      for (j = 0; j < BLOCKSIZE; j++)
	*work++ = data[b*BLOCKSIZE + i*ncol + j];
  }
}
