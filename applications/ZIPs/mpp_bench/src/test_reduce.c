//
// $Id: test_reduce.c,v 1.1 2008/08/27 22:05:30 
//

#include <mpp_bench.h>

int main (int argc, char *argv[])

{
  timer t;
  brand_t br;
  int64 seed, niters;
  int64 i, x, ans;

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
    // note that this barriers before & after reduce
    x = mpp_accum_long (i + MY_GTHREAD);
  timer_stop (&t);

  ans = (GTHREADS * (niters - 1)) + ((GTHREADS * (GTHREADS - 1)) / 2);
  if (x != ans) {
    printf ("ERROR: expected ans=%ld (got %ld) on PE %d\n", ans, x, MY_GTHREAD);
    mpp_exit(1);
  }
  
  if (MY_GTHREAD == 0) {
    printf ("performed %ld reductions in %9.3e cpu  secs (%9.3e /reduce)\n",
            niters, t.accum_cpus, t.accum_cpus / niters);
    printf ("performed %ld reductions in %9.3e wall secs (%9.3e /reduce)\n",
            niters, t.accum_wall, t.accum_wall / niters);
    printf ("ans = %ld\n", ans);
  }

 DONE:
  mpp_barrier_all();
  //mpp_finalize();
  return 0;
}
