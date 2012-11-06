//
// $Id: test_pingpong.c,v 1.6 2008/10/17 18:12:48 
//

#include <mpp_bench.h>

#if defined(__UPC__)
#define WAIT(X,VAL,ME) while((X)[ME]==(VAL))

#else
#define WAIT(X,VAL,ME) shmem_wait(X,VAL)

#endif

// x must be initialized to zero before call (preserved)

static void do_pingpong (volatile TYPE *x, int64 niters, int64 pair, int64 a, int64 b)

{
  timer t;
  static uint64 one = 1;
  int64 n, i, peer, me = MY_GTHREAD;
  uint64 *loc;

#if defined(__UPC__)
  loc = (uint64 *)&x[MY_GTHREAD];
  
#else
  loc = x;

#endif

  peer = (me == a) ? b : a;

  // do power of 2 iters up to niters
  for (n = 1; n < (2*niters); n *= 2) {
    if (n > niters)
      n = niters;
    i = n;
    
    timer_clear (&t);
    timer_start (&t);
    
    // b spins first
    if (me == b)
      {
	i--;
	WAIT (x, 0, me);
	*loc = 0;
      }
      
    // a puts first, spins last
    while (i-- > 0)
      {
	mpp_put (x, &one, 1, peer);
	WAIT (x, 0, me);
	*loc = 0;
      }
    
    // b puts last
    if (me == b)
      mpp_put (x, &one, 1, peer);
    
    timer_stop (&t);
    if (MY_GTHREAD == a)
      printf ("pair %7ld:%4ld ->%4ld: %7ld iters in %9.3e wall secs "
	      "(%9.3e /iter)\n", pair, a, b, n, t.accum_wall / 2,
	      t.accum_wall / 2 / n);
  }
}

#if defined(__UPC__)
//TYPE ball[THREADS];
volatile shared uint64 ball[THREADS];
 
#else
TYPE ball[1];

#endif

int main (int argc, char *argv[])

{
  brand_t br;
  int64 seed, npairs, niters;
  int64 i, a, b;

  start_pes(0);
  //mpp_init();

  if (argc < 4) {
    if (MY_GTHREAD == 0)
      fprintf (stderr, "Usage:\t%s seed npairs niters\n", argv[0]);
    goto DONE;
  }

  // must run on >1 PE
  if (GTHREADS < 2) {
    printf ("must use > 1 PEs !\n");
    mpp_exit(1);
  }

  // get args and seed prng
  seed = atol (argv[1]);
  if (MY_GTHREAD == 0)
    printf ("base seed is %ld\n", seed);
  brand_init (&br, seed);  // seed uniformly across PEs
  npairs = atol (argv[2]);
  niters = atol (argv[3]);

  // touch all memory, warm up interconnect
  do_warmup(&br);

  // init balls
#if defined(__UPC__)
  ball[MYTHREAD] = 0;

#else
  ball[0] = 0;
	
#endif

  for (i = 0; i < npairs; i++) {
    a = (brand(&br) >> 32) % GTHREADS;
    b = (brand(&br) >> 32) % GTHREADS;

    if (a == b) {
      i--;
      continue;
    }
    mpp_barrier_all();
    
    if ((MY_GTHREAD == a) || (MY_GTHREAD == b))
      do_pingpong (ball, niters, i, a, b);
  }
  
 DONE:
  mpp_barrier_all();
  //mpp_finalize();  /* Added this, else hangs with upc */
  return 0;
}
