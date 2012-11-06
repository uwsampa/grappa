/* CVS info                         */
/* $Date: 2010/04/02 20:45:41 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "xba.h"


int main (int argc, char *argv[])

{
  static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:45:41 $ $Revision: 1.5 $ $RCSfile: driver.c,v $ $Name:  $";

  brand_t br;
  timer t;
  int64 i, j, n;
  static uint64 mat[NROWS], ans[ITER];

  n = bench_init (&argc, argv, &br, &t, NULL);
  if (argc != 3) {
    printf ("invalid args\n");
    exit(1);
  }

  for (i = 0; i < NROWS; i++)  mat[i] = 0;
  for (i = 0; i < ITER;  i++)  ans[i] = 0;

  n /= NROWS;
  for (i = 0; i < n; i+= ITER) {
    for (j = 0; j < NROWS; j++)
      mat[j] = brand(&br); // new matrix
    timer_start (&t);
    xor_bit_arr (mat, &br, ans, MIN(ITER,n-i));
    timer_stop (&t);
  }

#ifndef __BMK_SHORT_FORMAT
  /* print out first/last 64 results */
  for (i = 0; i < 64; i++)
    printf ("%016lx  %016lx\n", ans[i], ans[ITER-1-i]);
#endif

#ifdef __BMK_SHORT_FORMAT
    set_status( 1 );  // Checked OK
#endif

  bench_end (&t, n*NROWS, "conditional sums");

  return 0;
}
