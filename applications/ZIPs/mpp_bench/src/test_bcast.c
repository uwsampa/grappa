//
// $Id: test_bcast.c,v 1.3 2008/08/26 19:44:44 
//

#include <math.h>
#include <mpp_bench.h>

static uint64 val;

static double scale_mem (double bytes, char **scale)

{
  int i;
  static char *units[8] = { "B",   "KiB", "MiB",
                            "GiB", "TiB", "PiB",
                            "EiB" }; // "ZiB", "YiB"
  
  if( scale == NULL )
    return bytes;

  i = 0;
  
  while ((bytes >= 1uL<<10) && units[i++]) {
    bytes /= 1024.0;
  }

  *scale = units[i];
  return bytes;
}

static void print_results (int64 nwrds, int64 reps, double w)

{
  double mem;
  char *scale;

  mem = (double)(nwrds * sizeof(uint64));
  mem = scale_mem (mem, &scale);
  printf ("performed %ld %3.0lf %3s bcasts in %9.3e wall secs (%9.3e /bcast)\n",
	  reps, mem, scale, w, w / (double)reps);
  printf ("%3.0lf %3s bcasts yield ", mem, scale);
  
  mem = (double)(reps * nwrds * sizeof(uint64)) / w;
  mem = scale_mem (mem, &scale);
  printf ("%6.2lf %3s/sec (/PE), ", mem, scale);
  
  mem = (double)((GTHREADS-1) * reps * nwrds * sizeof(uint64)) / w;
  mem = scale_mem (mem, &scale);
  printf ("%6.2lf %3s/sec (aggregate)\n", mem, scale);
}

// *dst and *src each of size nwrds uint64s

void do_bcast  (TYPE *dst, TYPE *src, int64 nwrds, brand_t *br, int64 msize,
		int64 rep, int64 fix_root)

{
  timer t;
  int64 i, n, root, max;
  uint64 x, cksum;
  uint64 *ldst, *lsrc;

  do_sync_init();

  // set pointers to local data
#if defined(__UPC__)
  ldst = (uint64 *)&dst[MY_GTHREAD];
  lsrc = (uint64 *)&src[MY_GTHREAD];
  
#else
  ldst = &dst[0];
  lsrc = &src[0];

#endif

  // touch all memory
  bzero ((void *)&ldst[0], nwrds * sizeof(uint64));
  bzero ((void *)&lsrc[0], nwrds * sizeof(uint64));
  
  // doing reps of powers-of-2 wrds up to max
  max = msize / sizeof(uint64);
  for (n = 1; n < (2 * max); n *= 2)
    {
      if (n > max)
	n = max;

      // fill src buff w/ data
      for (i = 1; i < n; i++) {
	x = brand(br) ^ val;
	lsrc[i] = x;
      }

      // Start Timing
      timer_clear (&t);
      timer_start (&t);
      for (i = 0; i < rep; i++) {
	// used fixed root if specified
	root = fix_root;
	if (root < 0)
	  root = brand(br) % GTHREADS;
	lsrc[0] = brand(br) ^ val;

	// broadcast n wrds from root to all PEs
	mpp_broadcast (dst, src, n, root);
      }
      mpp_barrier_all();
      timer_stop (&t);

      //
      // verify checksum on last iter
      //

      // compute global checksum
      x = 0;
      for (i = 0; i < n; i++)
	x += ldst[i];
      cksum = mpp_accum_long (x);
      // compare to local (should be x*GTHREADS)
      if ((GTHREADS * x) != cksum) {
	printf ("ERROR: expected bcast cksum %016lx (got %016lx)\n", 
		GTHREADS * x, cksum);
	mpp_exit(1);
      }
      mpp_barrier_all();

      if (MY_GTHREAD == 0) {
	//printf ("root %ld cksum is %016lx\n", root, cksum);
	print_results (n, rep, t.accum_wall);
      }
    }

  if (MY_GTHREAD == 0)
    printf ("cksum is %016lx\n", cksum);
}

int main (int argc, char *argv[])

{
  brand_t br;
  int64 i, seed, msize, niters, root = -1;
  const int64 nwrds = NWRDS / 2;
  double mem;
  char *scale;
  TYPE *dst, *src;

  start_pes(0);
  //mpp_init();

  if (argc < 4) {
    if (MY_GTHREAD == 0)
      fprintf (stderr, "Usage:\t%s seed msize(B) niters [root]\n", argv[0]);
    goto DONE;
  }

  // alloc two shared buffers
  // (mpp_alloc checks for valid pointer and casts)
  dst = mpp_alloc (nwrds * sizeof(uint64)); 
  src = mpp_alloc (nwrds * sizeof(uint64)); 

  // get args
  seed = atol (argv[1]);
  msize = atol (argv[2]);
  niters = atol (argv[3]);
  if (argc > 4)
    root = atol (argv[4]);

  // seed uniquely to generate a unique val /PE
  brand_init (&br, seed + ((int64)MY_GTHREAD << 32));
  val = brand(&br);
  // seed uniformly across PEs for benchmark
  brand_init (&br, seed);
  // runup a few times
  for (i = 0; i < 8; i++)  brand(&br);

  if (MY_GTHREAD == 0) {
    printf ("base seed is %ld\n", seed);
    mem = scale_mem (msize, &scale);
    printf ("msize = %.2lf %s\n", mem, scale);
  }
  if (msize < sizeof(uint64)) {
    if (MY_GTHREAD == 0)
      printf ("msize must be > %ld B\n", (int64)sizeof(uint64));
    goto DONE;
  }
  if (msize > (nwrds * sizeof(uint64))) {
    if (MY_GTHREAD == 0)
      printf ("msize must be < %ld B\n", nwrds * sizeof(uint64));
    goto DONE;
  }
  if (root >= GTHREADS)
    root = -1;
  if (MY_GTHREAD == 0) {
    if (root < 0)
      printf ("randomizing root PEs (%ld)\n", root);
    else
      printf ("using fixed root PE %ld\n", root);
  }

  // this exits on error
  do_bcast (dst, src, nwrds, &br, msize, niters, root);
  
  // free up the shared memory
  mpp_free (dst);
  mpp_free (src);

 DONE:
  mpp_barrier_all();
  //mpp_finalize();
  return 0;
}
