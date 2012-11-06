#include "bmm.h"

/* CVS info                      */
/* $Date: 2010/04/02 20:35:03 $  */
/* $Revision: 1.6 $              */
/* $RCSfile: bmm.c,v $           */
/* $State: Stab $:             */

/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:35:03 $ $Revision: 1.6 $ $RCSfile: bmm.c,v $ $Name:  $";

static void maketable (uint64 cols[], uint64 table[])

{
  int64 i, j;
  uint64 sum, a[16], b[16];

  sum  = 0;
  a[ 0] = sum;   sum ^= cols[7];  a[ 1] = sum;   sum ^= cols[6];
  a[ 3] = sum;   sum ^= cols[7];  a[ 2] = sum;   sum ^= cols[5];
  a[ 6] = sum;   sum ^= cols[7];  a[ 7] = sum;   sum ^= cols[6];
  a[ 5] = sum;   sum ^= cols[7];  a[ 4] = sum;   sum ^= cols[4];
  a[12] = sum;   sum ^= cols[7];  a[13] = sum;   sum ^= cols[6];
  a[15] = sum;   sum ^= cols[7];  a[14] = sum;   sum ^= cols[5];
  a[10] = sum;   sum ^= cols[7];  a[11] = sum;   sum ^= cols[6];
  a[ 9] = sum;   sum ^= cols[7];  a[ 8] = sum;

  sum  = 0;
  b[ 0] = sum;   sum ^= cols[3];  b[ 1] = sum;   sum ^= cols[2];
  b[ 3] = sum;   sum ^= cols[3];  b[ 2] = sum;   sum ^= cols[1];
  b[ 6] = sum;   sum ^= cols[3];  b[ 7] = sum;   sum ^= cols[2];
  b[ 5] = sum;   sum ^= cols[3];  b[ 4] = sum;   sum ^= cols[0];
  b[12] = sum;   sum ^= cols[3];  b[13] = sum;   sum ^= cols[2];
  b[15] = sum;   sum ^= cols[3];  b[14] = sum;   sum ^= cols[1];
  b[10] = sum;   sum ^= cols[3];  b[11] = sum;   sum ^= cols[2];
  b[ 9] = sum;   sum ^= cols[3];  b[ 8] = sum;

  for (i = 0; i < 16; i++)
    for (j = 0; j < 16; j++)
      table[(i << 4) + j] = b[i] ^ a[j];
}

static void bmm_ld (uint64 matrix[], uint64 table[])

{
  int64 i;
  static uint64 mtrans[64];

  for (i = 0; i < 64; i++)
    mtrans[i] = matrix[i];

  /* we'll omit tilting the matrix, since
   * that is tested in another benchmark
   tilt (mtrans);*/

  /* make tables on sets of 8 columns */
  for (i = 0; i < 8; i++)
    maketable (&mtrans[8*i], &table[256*i]);
}


#define BMM_LKUP(V,C)  (table[(((V) >> (56-8*(C))) & 0xff) + 256*(C)])
#define BMM_MX(V,ANS) \
{\
  uint64 fill = (V);\
  ANS  = BMM_LKUP(fill,0);\
  ANS ^= BMM_LKUP(fill,1);\
  ANS ^= BMM_LKUP(fill,2);\
  ANS ^= BMM_LKUP(fill,3);\
  ANS ^= BMM_LKUP(fill,4);\
  ANS ^= BMM_LKUP(fill,5);\
  ANS ^= BMM_LKUP(fill,6);\
  ANS ^= BMM_LKUP(fill,7);\
}

#define UPDATE(A) \
{\
  uint64 x=(A);\
  x |= x >> 32;\
  x &= x >> 16;\
  x = (x & 0xff) + ((x >> 8) & 0xff);\
  cnt[x]++;\
}

/* generate a random matrix
 * loop over vectors
 *   multiply vector by matrix
 *   reduce the result to 2 8-bit values
 *   accumulate sum of the bytes into a 512-long table
 */

void bmm_update (uint64 vec[], int32 cnt[], uint64 mat[])

{
  int64 i;
  static uint64 table[8*256];

  /* build 8 tables of 8-sums based on our matrix */
  bmm_ld (mat, table);

  for (i = 0; i < VLEN; i+=4) {
    uint64 a, b, c, d;

    BMM_MX(vec[i+0],a);
    BMM_MX(vec[i+1],b);
    BMM_MX(vec[i+2],c);
    BMM_MX(vec[i+3],d);

    UPDATE(a);
    UPDATE(b);
    UPDATE(c);
    UPDATE(d);
  }
  return;
}

