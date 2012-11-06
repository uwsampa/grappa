/* CVS info                         */
/* $Date: 2010/04/02 20:38:27 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "common_inc.h"
#include <gmp.h>

#include "brand.h"
#include "bm_timers.h"
#include "bench.h"
#include "error_routines.h"
#include "gmp_cksm_times.h"

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:38:27 $ $Revision: 1.5 $ $RCSfile: driver.c,v $ $Name:  $";

int main (int argc, char *argv[])

{
  timer t;
  int64 i, n;
  uint64 nbits=500;
  uint64 valid_cksum=0,cksum=0;
  brand_t br;
  mpz_t base, exp, mod, ans, sum;
  gmp_randstate_t rand_state;
  char answer_str[8096];

  n = bench_init (&argc, argv, &br, &t, "[size of arguments (in bits)]");
  if (argc >= 4){
    if ( (nbits = e_strtoul(argv[3]) ) ==  ULONG_MAX ){
      exit( EXIT_FAILURE );
    }
  }
#ifndef __BMK_SHORT_FORMAT
  printf("gmp_version=%s\n\n", gmp_version);
  printf ("nbits is %ld\n", nbits);
  printf ("mp_bits_per_limb = %d\n", mp_bits_per_limb);
#endif

  gmp_randinit    (rand_state, GMP_RAND_ALG_LC, 128UL);
  gmp_randseed_ui (rand_state, brand(&br));

  mpz_init(base);
  mpz_init(exp);
  mpz_init(mod);
  mpz_init(ans);
  mpz_init(sum);

  timer_start (&t);
  for (i = 0; i < n; i++) {
    mpz_urandomb (base, rand_state, nbits);
    mpz_urandomb (exp,  rand_state, nbits);
    mpz_urandomb (mod,  rand_state, nbits);

    /* force odd modulus */
    mpz_setbit (mod, 0);

    /* ans <- base ^ exp (Mod mod) */
    mpz_powm (ans, base, exp, mod);
    /* sum <- sum + ans */
    mpz_add  (sum, sum, ans);
  }

  timer_stop (&t);
#ifndef __BMK_SHORT_FORMAT
  printf ("sum of results is :\n  ");
  mpz_out_str (stdout, 10, sum);
  printf ("\n\n");
  printf ("nbits is %ld\n", nbits);
#endif

  // Use the argument to pick the check sum
  switch( nbits ){
  case 8192:
    //    valid_cksum = 0x00003238330d01030c04;
    valid_cksum = VALID_CKSM_GMP_8192;
    break;
  case 4096:
    //    valid_cksum = 0x00003b31370a0907050f;
    valid_cksum = VALID_CKSM_GMP_4096;
    break;
  case 2048:
    //    valid_cksum = 0x000007070931363d3532;
    valid_cksum = VALID_CKSM_GMP_2048;
    break;
  case 1024:
    //    valid_cksum = 0x00003a3f3335383d3534;
    valid_cksum = VALID_CKSM_GMP_1024;
    break;
  case 512:
    //  valid_cksum = 0x00000a03080c0f080134;
    valid_cksum = VALID_CKSM_GMP_512;
    break;
  case 256:
    //  valid_cksum = 0x00000a03080c0f080134;
    valid_cksum = VALID_CKSM_GMP_256;
    break;
  case 128:
    //    valid_cksum = 0x00000e0b0a063b3c363e;
    valid_cksum = VALID_CKSM_GMP_128;
    break;
  }

  (void) mpz_get_str (answer_str, 10, sum );
  // checksum the answer represented as a string
  for( i=0; i<strlen( answer_str ); i++){
    cksum = cksum ^ (  (uint64) answer_str[i] << (56-(i%8)*8) );
  }
#ifdef UPDATE_CKSM
  printf("Update VALID_CKSM_... to %020x\n", cksum);
#endif

#ifdef __BMK_SHORT_FORMAT
  set_status( 1 );  // Checked and correct
#endif
  if ( cksum != valid_cksum ){
#ifdef __BMK_SHORT_FORMAT
    set_status( 2 );  // Checked and wrong
#endif
    err_msg_ret(
		"ERROR: check sum is %020lx, should be %020lx\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		cksum, valid_cksum);
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }
  if ( t.accum_wall < TOO_FAST ){
    err_msg_ret(
		"ERROR: A wall time of %.2lf sec is impossible.\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		t.accum_wall );
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  bench_end (&t, n, "mpz_powm's");

  return 0;
}
