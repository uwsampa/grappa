/* CVS info                         */
/* $Date: 2010/04/02 20:43:31 $     */
/* $Revision: 1.6 $                 */
/* $RCSfile: driver.c,v $           */
/* $State: Stab $:                */

#include "bench.h"
#include <math.h>

/* Log base 2 of table size */
#ifndef LG
#define LG       25
#endif

#define INIT      1
#define RUN       2
#define CHECK     3
#define PRINT     4

#define NO_PRINT  0


static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:43:31 $ $Revision: 1.6 $ $RCSfile: driver.c,v $ $Name:  $";

uint64 a_sort (int64 op, brand_t*  br, int64 data_size);

int main (int argc, char *argv[])

{
  brand_t br;
  timer t;
  int64 n, n2, mhz = 0, ldata_size, data_size, next_size;
  uint64 pre_c_sum, post_c_sum, dum_c_sum;
  double w, cpu, step, ave = 0.0, total = 0.0;
  char c_buf[80];

  sprintf(c_buf," Tsize 16|%2d%s [chip Mhertz]",
     LG, (LG==25)?"":"(Test)");

  n2 = n = bench_init (&argc, argv, &br, &t, c_buf);

  ldata_size = atol(argv[3]);

  if ( ldata_size < 2 || ldata_size > LG){
    printf("ERROR: Sort size must be 16 or %d.\n", LG);
    exit(1);
  }

  if ( ldata_size != 16 && ldata_size != LG){
    printf("TEST: %ld is an unoffical size!\n", ldata_size);
  }

  if (argc == 5){
    mhz = atol(argv[4]);
  }

  data_size = 1 << ldata_size;

  if (n == 1){
    step = 0.0;
    next_size = data_size;
  }else{
    step = data_size/(sqrt(2.0)*(n-1));
    next_size = data_size*sqrt(2.0);
  }

  /* Initialize the data array */
  pre_c_sum = a_sort (INIT, &br, next_size);

  pre_c_sum = a_sort (RUN, &br, next_size);

  while (n-- > 0){
    if (n2 == 1){
      next_size = data_size;
    }else{
      next_size = data_size*sqrt(2.0) - step*(n2-n-1);
    }
    total += next_size;
    pre_c_sum = a_sort (INIT, &br, next_size);

    timer_start(&t);
    dum_c_sum = a_sort (RUN, &br, next_size);
    timer_stop(&t);

    // Check last and first
    if(n == (n2-1) || n == 1){
      post_c_sum = a_sort (CHECK, &br, next_size);
      if (pre_c_sum != post_c_sum){
#ifdef __BMK_SHORT_FORMAT
	set_status( 2 );  // Checked and incorrect
#else
	printf("ERROR:Checksums don't match\n%#018lx\n%#018lx\n",
	       pre_c_sum, post_c_sum);
#endif
      }else{
#ifdef __BMK_SHORT_FORMAT
	set_status( 1 );  // Checked and correct
#endif
      }
    }
  }
  timer_report (&t, &w, &cpu, NO_PRINT);

  pre_c_sum = a_sort (PRINT, &br, next_size);

#ifndef __BMK_SHORT_FORMAT
  ave = total/n2;
  printf("Ave sort size (2^^%ld)+%3.1f%% = %ld 64-bit words\n\n", ldata_size,
	 100*(ave-data_size)/data_size, (int64)ave);

  printf("%9.2e element/second\n\n", total/w);
  printf("%9.2e second/element\n\n", w/total);
  if(mhz != 0)
    printf("%7.2f cycle/element at %ld MHertz\n",
	   mhz*1000000*w/total, mhz);
#endif
  bench_end (&t, (int64)total, "words sorted");
  return 0;
}
