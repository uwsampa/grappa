/* CVS info                         */
/* $Date: 2010/04/02 20:44:16 $     */
/* $Revision: 1.7 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "common_inc.h"
#include "bench.h"
#include "error_routines.h"
#include "tilt_cksm_times.h"


/* if TILT is undef then link the C subroutine
 * otherwise compile with -DTILT="foo" where foo
 * is the proper name for calling the Fortran
 * tilt from C
 */

#ifndef TILT
#define TILT       tilt
#endif

/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:44:16 $ $Revision: 1.7 $ $RCSfile: driver.c,v $ $Name:  $";


void TILT (uint64 p[]);


int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 i, n;
  static uint64 arr[64];
  uint64 sum = 0;

  n = bench_init (&argc, argv, &br, &t, NULL);

  /* we force n odd */
  n |= 1;

#ifdef __BMK_SHORT_FORMAT
  set_status(1);   // Checked and correct
#else
  printf ("input matrix:\n\n");
#endif
  for (i = 0; i < 64; i++) {
    arr[i] = brand(&br);
    sum += arr[i];
#ifndef __BMK_SHORT_FORMAT
    printf ("arr[%2ld] = %016lx\n", i, arr[i]);
#endif
  }
#ifdef __BMK_SHORT_FORMAT
//    printf("%016lx\n", sum);
#  ifdef CSEED
  if (sum != VALID_CKSM1_CSEED_119554831){
#  else
  if (sum != VALID_CKSM1_FSEED_119554831){
#  endif
    set_status(2);
  }
#endif

  timer_start (&t);

  for (i = 0; i < n; i++)
    TILT (arr);

  timer_stop (&t);

#ifdef __BMK_SHORT_FORMAT
  for (i = 0; i < 64; i++)
    sum += arr[i];
//    printf("%016lx\n", sum);
#  ifdef CSEED
  if (sum != VALID_CKSM2_CSEED_119554831){
#  else
  if (sum != VALID_CKSM2_FSEED_119554831){
#  endif
    set_status(2);
  }
#else
  printf ("\noutput matrix:\n\n");
  for (i = 0; i < 64; i++)
    printf ("arr[%2ld] = %016lx\n", i, arr[i]);
#endif

  bench_end (&t, n, "tilts");

  return 0;
}
