/* CVS info                         */
/* $Date: 2010/04/02 20:36:10 $     */
/* $Revision: 1.6 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "common_inc.h"
#include "bench.h"
#include "error_routines.h"
#include "bs2_cksm_times.h"


/* tunable */

#define PAD      32L
#define NWORDS   4L               /* vector length for bit-serial computation
				   * (must divide TOT below)
				   * also must be equal to nwords parameter
				   * in the Fortran subroutine
				   */

#ifndef B_S_X2
#define B_S_X2 b_s_x2_
#endif

/* end tunable */

#define N        16L              /* size of our bit-serial "word" (bsw) */
#define VLEN     (N * NWORDS)     /* size of our short vector of bsw's   */
#define TOT      32L              /* total bsw's in memory at one time   */
#define TLEN     (N * TOT)        /* size of total bsw's                 */

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:36:10 $ $Revision: 1.6 $ $RCSfile: driver.c,v $ $Name:  $";

void B_S_X2 (uint64 x[], uint64 y[], int64 *n);
void tilt   (uint64 p[64]);

int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 i, j, k, n, dum;
  uint64 x;
  static uint64 tmp[64], test[64*TOT];
  static uint64 in[TLEN+PAD], out[TLEN];
  uint64 cksum = 0;

  if ( ( n = bench_init (&argc, argv, &br, &t, NULL) ) < 1){
    err_msg_exit("ERROR: Used %d iterations. Must < 1", n);
  }

  if (argc != 3)
    err_msg_exit("Usage: %s seed iter", argv[0]);

  for (i = 0; i < 64*TOT; i++)
    test[i] = (brand(&br) & _maskr(63)) % ((1L<<N)-1);

  for (i = 0; i < TOT; i++) {
    for (j = 0; j < 64; j++)
      tmp[j] = test[64*i+j];
    tilt (tmp);
    for (j = 0; j < N; j++)
      in[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS] = tmp[63-j];
#ifndef __BMK_SHORT_FORMAT
    printf ("  set %ld:\n\n", i);
    for (j = 0; j < N; j++)
      printf ("%2ld  %016lx\n", j,
	      in[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS]);
#endif
  }

  dum = N;
  for (j = 0; j < (TOT/NWORDS); j++)
    B_S_X2 (&in[j*VLEN], &out[j*VLEN], &dum);

  /* compute answers to check against */
  for (i = 0; i < 64*TOT; i++)
    test[i] = ((long)test[i] * (long)test[i]) % ((1L<<N)-1);

  for (i = 0; i < TOT; i++) {
#ifndef __BMK_SHORT_FORMAT
    printf ("  set %ld:\n\n", i);
    for (j = 0; j < N; j++)
      printf ("%2ld  %016lx\n", j,
	      out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS]);
#endif
    for (j = 0; j < N; j++)
      tmp[63-j] = out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS];
    for (; j < 64; j++)
      tmp[63-j] = 0;
    tilt (tmp);
    for (j = 0; j < 64; j++) {
      k = tmp[j]-test[64*i+j];
#ifndef __BMK_SHORT_FORMAT
      printf ("%2ld  %10ld  %10ld  %ld\n",
	      j, tmp[j], test[64*i+j], k);
#endif
    if (k) {
         err_msg_exit("ERROR: %s precheck mismatch\n", argv[0]);
      }
    }
  }

  /* now our timing runs */

  timer_start (&t);
  for (k = 0; k < n; k++) {
    x = brand(&br);
    for (i = 0; i < TLEN; i++)
      in[i] ^= x;

    for (j = 0; j < (TOT/NWORDS); j++)
      B_S_X2 (&in[j*VLEN], &out[j*VLEN], &dum);
  }
  timer_stop (&t);

  /* print out last result */
  i = TOT-1;
  for (j = 0; j < N; j++){
#ifndef __BMK_SHORT_FORMAT
    printf ("%2ld  %016lx\n", j,
	    out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS]);
#endif
    cksum ^= out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS] ;
  }

#ifdef UPDATE_CKSM
  printf("Update VALID_CKSM_55179 to %020x\n", cksum);
#endif
#ifdef __BMK_SHORT_FORMAT
  set_status( 1 );  // Checked and correct
#endif
  if ( cksum != VALID_CKSM_257027 ){
#ifdef __BMK_SHORT_FORMAT
    set_status( 2 );  // Checked and wrong
#endif
    err_msg_ret(
		"ERROR: check sum is %020lx, should be %020lx\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		cksum, VALID_CKSM_257027);
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  if ( t.accum_wall < TOO_FAST_257027 ){
    err_msg_ret(
		"ERROR: A wall time of %.2lf sec is impossible.\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		t.accum_wall );
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  bench_end (&t, n*TOT*64, "bit-serial squares");

  return 0;
}
