/* CVS info                         */
/* $Date: 2010/04/02 20:37:55 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "common_inc.h"
#include "bench.h"
#include "error_routines.h"
#include "cba_cksm_times.h"

#include "cba.h"
#include <assert.h>

/*@unused@*/

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:37:55 $ $Revision: 1.5 $ $RCSfile: driver.c,v $ $Name:  $";

int64 testit (brand_t *br, uint64 *data, uint64 *chk, uint64 *work,
	      uint64 *out, int64 nrow, int64 ncol);

int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 i, j, n, nrow, ncol, niters;
  uint64 *data, *chk, *work, *out;
  int64 failed;
  double shortest_time;

  n = bench_init (&argc, argv, &br, &t, "nrow ncol");
  n *= 64;  /* we'll do iterations in blocks of 64 */
  if (argc != 5) {
    err_msg_exit("Usage: %s seed iter nrow ncol", argv[0]);
  }
  //  nrow = atol(argv[3]);
  if ( (nrow = e_strtoul(argv[3]) ) ==  ULONG_MAX ){
    exit( EXIT_FAILURE );
  }
  //ncol = atol(argv[4]);
  if ( ( ncol = e_strtoul(argv[4]) ) ==  ULONG_MAX ){
    exit( EXIT_FAILURE );
  }

  if ((ncol % BLOCKSIZE) != 0) {
    err_msg_exit("ERROR: BLOCKSIZE (%ld) must divide ncol (%ld)\n",
	    BLOCKSIZE, ncol);
  }
  assert ((NITERS % 64) == 0);

  work = (uint64 *) calloc (  (size_t)((nrow*ncol + PAD + NITERS) * 2),
			      sizeof(uint64));
  if (work == NULL) {
    err_msg_exit("ERROR: Can't calloc, out of memory\n",
	    BLOCKSIZE, ncol);
  }
  out  = &work[nrow*ncol + PAD];
  data = &out [NITERS];
  chk  = &data[nrow*ncol + PAD];

  for (i = 0; i < (nrow*ncol); i++)
    data[i] = brand(&br);
  blockit (data, nrow, ncol, work);

  for (i = 0; i < n; i+=NITERS) {
    niters = n - i;
    if (niters > NITERS)
      niters = NITERS;

    for (j = 0; j < niters; j++) {
      /* pick NITERS random rows in the range 1..(nrow-1) */
      out[j] = 1 + (brand(&br) % (nrow-1));
      out[j] <<= 48;  /* store index in high 16 bits */
    }

    timer_start (&t);
    (void)cnt_bit_arr (work, nrow, ncol, out, niters);
    timer_stop  (&t);
  }

  /* print out last 64 results */
#ifndef __BMK_SHORT_FORMAT
  printf ("\nlast 64 results:\n\n");
  for (i = niters-64; i < niters; i++) {
    printf (" %9lu", (unsigned long)out[i]);
    if ((i%8) == 7)
      printf ("\n");
  }
#endif

  /* test correctness */
  for (i = 0; i < (nrow*ncol); i++)
    data[i] = brand(&br);
  blockit (data, nrow, ncol, work);
  failed = testit (&br, data, chk, work, out, nrow, ncol);
  if ( failed == 0 )
    failed = testit (&br, data, chk, work, out, nrow, ncol);
#ifdef __BMK_SHORT_FORMAT
  set_status( 1 );  // Checked and correct
#endif
  if ( failed ){
#ifdef __BMK_SHORT_FORMAT
    set_status( 2 );  // Checked and wrong
#endif
    err_msg_ret(
		"ERROR: %ld failures\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		failed);
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }
  if( nrow==64 && ncol==64 ){
    shortest_time = TOO_FAST_64_64_7173289;
  }
  if( nrow==145 && ncol==2368 ){
    shortest_time = TOO_FAST_145_2368_202323;
  }
  if( nrow==231 && ncol==28000 ){
    shortest_time = TOO_FAST_231_128000_3400;
  }

  if ( t.accum_wall < shortest_time ){
    err_msg_ret(
		"ERROR: A wall time of %.2lf sec is impossible.\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		t.accum_wall );
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  bench_end (&t, n*ncol, "64-bit counts");

  return 0;
}

int64 testit (brand_t *br, uint64 *data, uint64 *chk, uint64 *work,
	     uint64 *out, int64 nrow, int64 ncol)

{
  int64 i, fail=0;

  for (i = 0; i < NITERS; i++) {
    /* pick NITERS random rows in the range 1..(nrow-1) */
    chk[i] = 1 + (brand(br) % (nrow-1));
    chk[i] <<= 48;
    out[i] = chk[i];
  }

  (void)cnt_bit_arr_nb (data, nrow, ncol, chk, NITERS);
  (void)cnt_bit_arr    (work, nrow, ncol, out, NITERS);
  for (i = 0; i < NITERS; i++)
    if ((chk[i] != out[i]) || (out[i] <= 0)) {
      err_msg_ret ("ERROR: result %ld failed (chk = %lu  out = %lu)\n",
	      i, (unsigned long)chk[i], (unsigned long)out[i]);
      fail++;
    }

  //  err_msg_ret ("there were %ld failures\n", fail);

  return fail;
}
