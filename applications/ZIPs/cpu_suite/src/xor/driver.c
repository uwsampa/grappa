/* CVS info                         */
/* $Date: 2010/04/02 20:46:45 $     */
/* $Revision: 1.7 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "bench.h"
#include "error_routines.h"
#include "xor_cksm_times.h"

/* Array size in words */
#define A_SIZE                  (1L<<25)

#define INIT      1
#define RUN       2
#define CHECK     3

#define NO_PRINT  0


static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:46:45 $ $Revision: 1.7 $ $RCSfile: driver.c,v $ $Name:  $";

void xor (int64 op, brand_t br, int64 array_size);


int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 t_bytes, n, n2;
  double w, cpu;

  int64 array_size, mhz = 0;

  n2 = n = bench_init (&argc, argv, &br, &t, "[Array_length [chip MHertz]]");

  if (argc < 4){
    array_size = A_SIZE;
  }
  else if ( (array_size = (int64)atol(argv[3])) > A_SIZE )
  {
    err_msg_exit("ERROR: Memory constant A_SIZE = %ld too small\n", A_SIZE);
  }

#ifndef __BMK_SHORT_FORMAT
  printf("Array lengths are %ld\nTotal memory size is %ld words %ld bytes \n\n",
	 array_size, 3*array_size, 8*3*array_size);
#endif

  if (argc == 5){
    mhz = atol(argv[4]);
  }

  /* Initialize arrays a,b and c */

  xor(INIT, br, array_size);

  xor(RUN, br, array_size);

  timer_start(&t);

  while (n-- > 0)
    xor(RUN, br, array_size);

  timer_stop(&t);

  xor(CHECK, br, array_size);

  /* Total number of bytes moved */
  t_bytes =  n2 * (array_size*3) * sizeof(uint64);

  timer_report (&t, &w, &cpu, NO_PRINT);

  if(w == 0.0 || cpu == 0.0){
    err_msg_exit("ERROR: Problem too small to measure time!\n\n");
  }

#ifndef __BMK_SHORT_FORMAT
  printf("%9.2f MB/w_s\n", t_bytes/(w*(1<<20)));
  printf("%9.2f MW/w_s\n", t_bytes/((w*(1<<20))*sizeof(uint64)));
  if(mhz != 0){
    printf("%9.2f cycles/mem_op at %ld MHertz\n",
	   (mhz*1000000.*w)/(t_bytes/sizeof(uint64)), mhz);
    printf("%9.2f mem_op/cycle at %ld MHertz\n",
	   (t_bytes/sizeof(uint64))/(mhz*1000000.*w), mhz);
  }
#endif

#ifdef __BMK_SHORT_FORMAT
    set_status( 1 );  // Checked OK
#endif

  bench_end (&t, t_bytes, "Bytes");
  return 0;
}
