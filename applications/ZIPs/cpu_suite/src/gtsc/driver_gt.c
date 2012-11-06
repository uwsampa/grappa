/* CVS info                         */
/* $Date: 2010/04/02 20:38:54 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: driver_gt.c,v $        */
/* $State: Stab $:                */

#include "common_inc.h"
#include "bench.h"
#include "error_routines.h"
#include "gtsc_cksm_times.h"

/* Log base 2 of data array size */
#ifndef LG
#define LG                            25
#endif

#define MAX_ITER                      17

#define INIT                           1
#define RUN8                           2
#define RUN16                          3
#define RUN25                          4

#define NO_PRINT                       0


/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:38:54 $ $Revision: 1.5 $ $RCSfile: driver_gt.c,v $ $Name:  $";

uint64 gs (int64 op, brand_t* br);

int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 t_bytes, t_loads, n, n2, mhz = 0;
  int64 m_loads, loads_per_iter ;
  int64 op, lg2_array_size, array_size, in_loop_cnt;
  uint64 c_sum, valid_cksum;
  double w, cpu;
  char c_buf[80];

  sprintf(c_buf," Tsize 8|16|%2d%s [chip Mhertz]",
     LG, (LG==25)?"":"(Test)");

  n = bench_init (&argc, argv, &br, &t, c_buf);
  n2 = n;

  if ( (op = (int64)e_strtoul(argv[3]) ) ==  ULONG_MAX ){
    exit( EXIT_FAILURE );
  }
  if ( op != 8 && op != 16 && op != LG){
    err_msg_exit("ERROR: Tsize must be 8,16 or %d.\n", LG);
  }

  if (argc == 5){
    if ( (mhz = (int64)e_strtoul(argv[4]) ) ==  ULONG_MAX ){
      exit( EXIT_FAILURE );
    }
  }

  /* Initialize the data array */
  c_sum = gs (INIT,  &br);

#ifdef __BMK_SHORT_FORMAT
  set_status( 1 );  // Checked and correct
#endif

  switch (op) {
  case 8:
    // Warmup gs
    c_sum ^= gs (RUN8,  &br);
    timer_start(&t);

    while (n-- > 0)
      c_sum ^= gs (RUN8, &br);

    timer_stop(&t);

    m_loads = 4;
    lg2_array_size = 8;
    array_size = (1<<lg2_array_size);
    in_loop_cnt = array_size;
    //    valid_cksum = 0xc6fe43ba87ec0edb;
    valid_cksum = VALID_CKSM_GT_8;

    break;

  case 16:
    // Warmup gs
    c_sum ^= gs (RUN16,  &br);
    timer_start(&t);

    while (n-- > 0)
      c_sum ^= gs (RUN16, &br);

    timer_stop(&t);

    m_loads = 1;
    lg2_array_size = 16;
    array_size = (1<<lg2_array_size);
    in_loop_cnt = array_size;
    //    valid_cksum = 0x93918dc4af3ca0ff;
    valid_cksum = VALID_CKSM_GT_16;
    break;

  case LG:
    // Warmup gs
    c_sum ^= gs (RUN25,  &br);
    timer_start(&t);

    while (n-- > 0)
      c_sum ^= gs (RUN25, &br);

    timer_stop(&t);

    m_loads = 1;
    lg2_array_size = LG;
    array_size = (1<<lg2_array_size);
    in_loop_cnt = (1<<MAX_ITER);
    //    valid_cksum = 0xca7dc734a6a9587f;
    valid_cksum = VALID_CKSM_GT_LG;
    break;
  }

#ifndef __BMK_SHORT_FORMAT
  printf("Valid checksum = %#018lx\n", valid_cksum);
  printf("Checksum       = %#018lx\n\n", c_sum);
#endif

  /* = macros/inner_iter * loads/macro_loops * macro_loops * inner_iter/iter */
  loads_per_iter = 24 * m_loads * (64/lg2_array_size) * in_loop_cnt;

  /* Total number of loads = iter * loads/iter */
  t_loads = n2 * loads_per_iter;

  /* Total number of bytes moved = loads * bytes/load */
  t_bytes = t_loads*sizeof(uint64);

  timer_report (&t, &w, &cpu, NO_PRINT);
#ifndef __BMK_SHORT_FORMAT
  printf("data array is 2^%ld = (%ld) words long\n",
	 lg2_array_size, array_size);

  if(mhz != 0){
   printf("%10.2f cycles/load at %ld MHertz\n",
	  (mhz*w*(1L<<20))/t_loads, mhz);
  }
#endif
  if ( c_sum != valid_cksum ){
#ifdef __BMK_SHORT_FORMAT
    set_status( 2 );  // Checked and wrong
#endif
    err_msg_ret(
		"ERROR: check sum is %020lx, should be %020lx\n"
		"Changing wall time to 200000000.\n"
		"Changing cpu time to 100000000.\n",
		c_sum, valid_cksum);
    t.accum_wall = 200000000.;
    t.accum_cpus = 100000000.;
  }

  bench_end (&t, t_bytes, "Bytes");
  return 0;
}
