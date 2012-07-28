//
// $Id: test_barrier.c,v 1.3 2008/08/27 22:00:40 
//

#include <mpp_bench.h>

int main (int argc, char *argv[])

{
  timer t;
  brand_t br;
  int64 seed, niters;
  int64 i;

  start_pes(0);
  //mpp_init();

  if (argc < 2) {
    if (MY_GTHREAD == 0)
      fprintf (stderr, "Usage:\t%s niters\n", argv[0]);
    goto DONE;
  }

  // get args and seed prng
  seed = time(NULL);
  if (MY_GTHREAD == 0)
    printf ("base seed is %ld\n", seed);
  brand_init (&br, seed);  // seed uniformly across PEs
  niters = atol (argv[1]);

  // touch all memory, warm up interconnect
  do_warmup(&br);

  // Start timing
  timer_clear (&t);
  timer_start (&t);
  for (i = 0; i < niters; i++)
    mpp_barrier_all();
  timer_stop (&t);

  if (MY_GTHREAD == 0) {
    printf ("performed %ld barriers in %9.3e cpu  secs (%9.3e /barrier)\n",
	    niters, t.accum_cpus, t.accum_cpus / niters);
    printf ("performed %ld barriers in %9.3e wall secs (%9.3e /barrier)\n",
	    niters, t.accum_wall, t.accum_wall / niters);
  }

 DONE:
  mpp_barrier_all();
  //mpp_finalize();
  return 0;
}
