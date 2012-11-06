//
// $Id: mpp_bench.c,v 1.7 2008/08/27 21:39:17 
//

#include <mpp_bench.h>

//
// timing
//

#include <sys/time.h>

// wall-clock time

static double wall (void)
     
{
  struct timeval tp;
  
  gettimeofday (&tp, NULL);
  return
    tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

#include <sys/resource.h>

// cpu + system time

static double cpus (void)

{
  struct rusage ru;
  
  getrusage(RUSAGE_SELF,&ru);
  return
    (ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
    (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec)/(double)1.0e6;
}

void timer_clear     (timer *t)

{
  t->accum_wall = t->accum_cpus = 0;
  t->start_wall = t->start_cpus = 0;
  t->running = 0;
}

void timer_start     (timer *t)

{
  t->start_wall = wall();
  t->start_cpus = cpus();
  t->running = 1;
}

void timer_stop      (timer *t)

{
  if (t->running == 0)
    return;
  t->accum_cpus += cpus() - t->start_cpus;
  t->accum_wall += wall() - t->start_wall;
  t->running = 0;
}

void timer_report    (timer *t, double *pwall, double *pcpus, int64 print)

{
  double w, c;

  w = t->accum_wall;
  c = t->accum_cpus;

  if (t->running) {
    c += cpus() - t->start_cpus;
    w += wall() - t->start_wall;
  }
  if (print) {
    printf ("%7.3f secs of wall clock time\n",     w);
    printf ("%7.3f secs of cpu and system time\n", c);
  }

  if (pwall)  *pwall = w;
  if (pcpus)  *pcpus = c;
}

//
// PRNG
//

uint64 brand         (brand_t *p)

{
  uint64 hi=p->hi, lo=p->lo, i=p->ind, ret;

  ret = p->tab[i];

  _BR_64STEP_(hi,lo,45,118);

  p->tab[i] = ret + hi;

  p->hi  = hi;
  p->lo  = lo;
  p->ind = hi & _maskr(_BR_LG_TABSZ_);

  return ret;
}

void   brand_init    (brand_t *p, uint64 val)

{
  int64 i;
  uint64 hi, lo;

  hi = 0x9ccae22ed2c6e578uL ^ val;
  lo = 0xce4db5d70739bd22uL & _maskl(118-64);

  for (i = 0; i < 64; i++)
    _BR_64STEP_(hi,lo,33,118);
  
  for (i = 0; i < _BR_TABSZ_; i++) {
    _BR_64STEP_(hi,lo,33,118);
    p->tab[i] = hi;
  }
  p->ind = _BR_TABSZ_/2;
  p->hi  = hi;
  p->lo  = lo;

  for (i = 0; i < _BR_RUNUP_; i++)
    brand(p);
}

//
// upc/shmem functionality
//

TYPE *mpp_alloc      (int64 nbytes)

{
  TYPE *ptr;
#if defined(__UPC__)
//  ptr = (TYPE *)upc_all_alloc (nbytes * GTHREADS, sizeof(uint64));
  ptr = (TYPE *)upc_all_alloc (nbytes/sizeof(uint64) * GTHREADS, sizeof(uint64));
  if (ptr == NULL) {
    printf ("ERROR: upc_all_alloc of %ld bytes failed !\n", nbytes * GTHREADS);
    mpp_exit(1);
  }

#else
  ptr = (TYPE *)shmalloc (nbytes);
  if (ptr == NULL) {
    printf ("ERROR: shmalloc of %ld bytes failed !\n", nbytes);
    mpp_exit(1);
  }

#endif
  
  return ptr;
}

void  mpp_free       (TYPE *ptr)

{
  mpp_barrier_all();
  
#if defined(__UPC__)
  if (MY_GTHREAD == 0)
    upc_free (ptr);

#else
  shfree (ptr);

#endif
}

#if defined(__UPC__)
int64 mpp_accum_long (int64 val)

{
  shared static uint64 src[THREADS];
  shared static uint64 dst;

  src[MYTHREAD] = val;

  upc_all_reduceUL (&dst, src, UPC_ADD, THREADS, 1, NULL,
		    UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC);
  return dst;
}

#else
static int64 Sync[_SHMEM_REDUCE_SYNC_SIZE];
static int64 Work[2 + _SHMEM_REDUCE_MIN_WRKDATA_SIZE];
static int64 Sync_init=0;

int64 mpp_accum_long (int64 val)

{
  int64 i;
  static int64 target, source;

  do_sync_init();
  source = val;

  // barrier to ensure Sync not in use
  shmem_barrier_all();

  shmem_long_sum_to_all (&target, &source, 1, 0, 0, GTHREADS, Work, Sync);

  shmem_barrier_all();
  return target;
}

#endif

void  do_sync_init   (void)

{
#if !defined(__UPC__)
  int64 i;

  if (! Sync_init) {
    // need to initialize Sync first time around
    for (i = 0; i < _SHMEM_REDUCE_SYNC_SIZE; i++)
      Sync[i] = _SHMEM_SYNC_VALUE;
    Sync_init = 1;
  }

#endif
  mpp_barrier_all();
}

void  mpp_put        (volatile TYPE *dst, uint64 *src, int64 nelem, int64 pe)

{
#if defined(__UPC__)
  shared [0] uint64 *ptr = (shared [0] uint64 *)&dst[pe];
  upc_memput (ptr, src, nelem * sizeof(uint64));

#else
  shmem_put (dst, src, nelem, pe);

#endif
}

// NOTE: must have called do_sync_init() first

void  mpp_broadcast  (TYPE *dst, TYPE *src, int64 nelem, int64 root)

{
#if defined(__UPC__)
  upc_all_broadcast (dst, &src[root], nelem * sizeof(uint64),
		     UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC);

#else
  // barrier to ensure Sync not in use
  mpp_barrier_all();

  // shmem_broadcast* does not copy on root
  if (MY_GTHREAD == root)
    memcpy (dst, src, nelem * sizeof(uint64));

  shmem_broadcast64 (dst, src, nelem, root, 0, 0, GTHREADS, Sync);
  
  // for Quadrics SHMEM (XC) no barrier necessary post-bcast
  // (libelan broadcast function is called)
#if !defined(XC)
  mpp_barrier_all();
#endif

#endif
}

// your choice of permutations indexed by ITER
// use one below or write your own PERM() macro

// #define PERM(ME,TOT,ITER) ((ME)^(ITER)) // ok if 2^n pes
#define PERM(ME,TOT,ITER) (((ME)+(ITER))%(TOT)) // ok
// #define PERM(ME,TOT,ITER) (((ME)+(TOT)-(ITER))%(TOT)) // ok

#define MAX(A,B) (((A)>(B)) ? (A) : (B))
#define MIN(A,B) (((A)<(B)) ? (A) : (B))

// returns words sent per PE

int64  all2all       (TYPE *dst, uint64 *src, int64 len, int64 nwrd)

{
  int64 i, j, pe;
  
  len = len - (len % (nwrd * GTHREADS)); // force even multiple
  for (i = 0; i < len; i += GTHREADS*nwrd)
    {
      for (j = 0; j < GTHREADS; j++) {
	pe = PERM (MY_GTHREAD, GTHREADS, j);

#if defined(__UPC__)
	shared [0] uint64 *ptr = (shared [0] uint64 *)&dst[pe];
	upc_memput (&ptr[i + MY_GTHREAD*nwrd], &src[i + pe*nwrd],
		    nwrd * sizeof(uint64));
	
#else
	shmem_put (&dst[i + MY_GTHREAD*nwrd], &src[i + pe*nwrd], nwrd, pe);
	
#endif
      }
    }
  return len;
}

static uint64 do_cksum (uint64 *arr, int64 len)

{
  int64 i, cksum = 0;

  for (i = 0; i < len; i++)
    cksum += arr[i];
  return mpp_accum_long (cksum);
}

// exits on error

void  do_all2all     (TYPE *tab, uint64 *loc, brand_t *br, int64 msize,
		      int64 tsize, int64 rep, int64 print)

{
  timer t;
  double nsec;
  int64 i, len, cksum;

  while (rep-- > 0) {
    // length in words
    len = tsize / sizeof(uint64);
    for (i = 0; i < len; i += 64)
      loc[i] = brand(br);
    // we'll have destination/source arrays each of half size
    len /= 2;
    // force even multiples
    len = len - (len % (msize/sizeof(uint64) * GTHREADS)); 

    // source checksum
    cksum = do_cksum (&loc[len], len);
    if ((MY_GTHREAD == 0) && print)
      printf ("cksum is %016lx\n", cksum);
    
    // Start Timing
    timer_clear (&t);
    timer_start (&t);
    len = all2all (&tab[0], &loc[len], len, msize/sizeof(uint64));
    mpp_barrier_all();
    timer_stop (&t);
    // End Timing
    
    // dest checksum
    i = do_cksum (&loc[0], len);
    if (i != cksum) {
      printf ("PE %4ld  ERROR: %016lx != %016lx\n", (int64)MY_GTHREAD, i,
	      cksum);
      mpp_exit(1);
    }
    
    if ((MY_GTHREAD == 0) && print) {
      printf ("wall reports %8.3f secs  cpus report %8.3f secs\n",
	      t.accum_wall, t.accum_cpus);
      nsec = MAX(t.accum_wall, t.accum_cpus);
      if (nsec > 0)
	printf ("%8.3f MB/sec with %ld bytes transfers\n",
		len*sizeof(uint64)/(double)(1L<<20)/nsec, msize);
    }
  }

}

// non-zero return on error

void  do_warmup      (brand_t *br)

{
  TYPE *tab;
  uint64 *loc;
  
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

  // all2all to warm-up interconnect
  do_all2all (tab, loc, br, (1uL<<12), NWRDS*sizeof(uint64), 8, 0);

  // free up the shared memory
  mpp_free (tab);
  mpp_barrier_all();
}
