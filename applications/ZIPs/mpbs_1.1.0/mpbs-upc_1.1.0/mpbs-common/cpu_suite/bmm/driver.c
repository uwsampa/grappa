/* CVS info                      */
/* $Date: 2010/04/02 20:35:32 $  */
/* $Revision: 1.6 $              */
/* $RCSfile: driver.c,v $        */
/* $State: Stab $:             */

#include "common_inc.h"
#include "bench.h"
#include "error_routines.h"
#include "bmm.h"
#include "bmm_cksm_times.h"

/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:35:32 $ $Revision: 1.6 $ $RCSfile: driver.c,v $ $Name:  $";

int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 i, j, n;
  static uint64 vec[VLEN], mat[64];
  static int32 cnt[CLEN];
  int32 cksum = 0;

  if ( ( n = bench_init (&argc, argv, &br, &t, NULL) ) != 55179){
    err_msg_exit("ERROR: Used %d iterations. Must use 55179", n);
  }

  if (argc != 3)
    err_msg_exit("Usage: %s seed iter", argv[0]);

  for (i = 0; i < VLEN; i++)
    vec[i] = brand(&br);

  timer_start (&t);

  for (i = 0; i < n; i++) {
    for (j = 0; j < 64; j++)
      mat[j] = brand(&br);
    bmm_update (vec, cnt, mat);
  }

  timer_stop (&t);

  /* check cnts */
  for (i = 0; i < CLEN; i++) {
#ifndef __BMK_SHORT_FORMAT
    if ((i%8) == 0)
      printf ("%4ld: ", i);
    printf (" %8d", cnt[i]);
    if ((i%8) == 7)
      printf ("\n");
#endif
    cksum ^= cnt[i];
  }
#ifdef __BMK_SHORT_FORMAT
  set_status( 1 );  // Checked and correct
#endif
#ifdef UPDATE_CKSM
  printf("Update VALID_CKSM_55179 to %020x\n", cksum);
#endif

  if ( cksum != VALID_CKSM_55179 ){
#ifdef __BMK_SHORT_FORMAT
    set_status( 2 );  // Checked and wrong
#endif
    err_msg_ret(
		"ERROR: check sum is %020lx, should be %020lx\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		cksum, VALID_CKSM_55179);
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  if ( t.accum_wall < TOO_FAST_55179 ){
#ifdef __BMK_SHORT_FORMAT
  set_status( 3 );  // Checked and too fast
#endif
    err_msg_ret(
		"ERROR: A wall time of %.2lf sec is impossible.\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		t.accum_wall );
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  bench_end (&t, n*VLEN, "bit matrix multiplies");

  return 0;
}

