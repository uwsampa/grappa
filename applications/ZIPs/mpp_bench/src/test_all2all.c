// driver.c

#include <strings.h>
#include <mpp_bench.h>

int main (int argc, char *argv[])

{
  brand_t br;
  int64 seed, arg, msize, tsize, rep;
  TYPE *tab;
  uint64 *loc;

  start_pes(0);
  //mpp_init();

  if (argc < 5) {
    if (MY_GTHREAD == 0)
      fprintf (stderr, "Usage:\t%s seed msg_size(B) table_size(MB) rep_cnt "
        "[ms2 ts2 rc2 ..]\n", argv[0]);
      goto DONE;
  }

  // alloc some shared space
  // (checks for valid pointer and casts)
  tab = mpp_alloc (NWRDS * sizeof(uint64));

  // pointer to local space
#if defined(__UPC__)
  loc = (uint64 *)&tab[MY_GTHREAD];

#else
  loc = &tab[0];

#endif

  // init all local memory
  bzero ((void *)&loc[0], NWRDS * sizeof(uint64));

  seed = atol (argv[1]);
  if (MY_GTHREAD == 0)
    printf ("base seed is %ld\n", seed);
  seed += (uint64)MY_GTHREAD << 32;
  brand_init (&br, seed);  // seed uniquely per PE

  arg = 2;
  while (arg < argc) {
    msize = atol (argv[arg++]);
    if (arg >= argc)
      break;
    tsize = atol (argv[arg++]) * (1L << 20);
    if (arg >= argc)
      break;
    rep   = atol (argv[arg++]);

    if (MY_GTHREAD == 0)
      printf ("tsize = %ldMB  msize = %5ldB\n", tsize/(1L<<20), msize);
    if (msize < sizeof(long)) {
      if (MY_GTHREAD == 0)
	printf ("msize must be > %ld B\n", (int64)sizeof(long));
      goto DONE;
    }
    if (tsize > (NWRDS * sizeof(long))) {
      if (MY_GTHREAD == 0)
	printf ("tsize must be < %ld MiB\n",
		(int64)(NWRDS * sizeof(long)) / (1uL<<20));
      goto DONE;
    }

    // exits on error
    do_all2all (tab, loc, &br, msize, tsize, rep, 1);
    
    if (MY_GTHREAD == 0)
      printf ("\n");
  }

  // free up the shared memory
  mpp_free (tab);

 DONE:
  mpp_barrier_all();
  //mpp_finalize();
  return 0;
}
