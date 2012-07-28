#ifndef BENCH_H
#define BENCH_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* LP64 types */

typedef   signed  char  int8 ;
typedef unsigned  char uint8 ;
typedef          short  int16;
typedef unsigned short uint16;
typedef            int  int32;
typedef unsigned   int uint32;
typedef           long  int64;
typedef unsigned  long uint64;

/* timing */

#include <sys/time.h> 
#include <time.h>

/* wall-clock time */

static double wall(void)
     
{
  struct timeval tp;
  
  gettimeofday (&tp, NULL);
  return
    tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

#include <sys/resource.h>

/* cpu + system time */

static double cpus(void)

{
  struct rusage ru;
  
  getrusage(RUSAGE_SELF,&ru);
  return
    (ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
    (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec)/(double)1.0e6;
}

typedef struct {
  double accum_wall, accum_cpus;
  double start_wall, start_cpus;
  time_t init_time;
  char running;
} timer;

static void timer_clear (timer *t)

{
  t->accum_wall = t->accum_cpus = 0;
  t->start_wall = t->start_cpus = 0;
  t->running = 0;
}


#if defined(__GNUC__)
static void timer_start (timer *t)  __attribute__ ((unused));
#endif
static void timer_start (timer *t)

{
  t->start_wall = wall();
  t->start_cpus = cpus();
  t->running = 1;
}

#if defined(__GNUC__)
static void timer_stop  (timer *t) __attribute ((unused));
#endif
static void timer_stop  (timer *t)

{
  if (t->running == 0)
    return;
  t->accum_cpus += cpus() - t->start_cpus;
  t->accum_wall += wall() - t->start_wall;
  t->running = 0;
}

static void timer_report (timer *t, double *pwall, double *pcpus,
			  int64 print)

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


/* some masking macros */

#define _ZERO64       0uL
#define _maskl(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64-(x))))
#define _maskr(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64-(x))))
#define _mask(x)      (((x) < 64) ? _maskl(x) : _maskr(2*64 - (x)))

/* PRNG */

#define _BR_RUNUP_      128L
#define _BR_LG_TABSZ_    7L
#define _BR_TABSZ_      (1L<<_BR_LG_TABSZ_)

typedef struct {
  uint64 hi, lo, ind;
  uint64 tab[_BR_TABSZ_];
} brand_t;

#define _BR_64STEP_(H,L,A,B) {\
  uint64 x;\
  x = H ^ (H << A) ^ (L >> (64-A));\
  H = L | (x >> (B-64));\
  L = x << (128 - B);\
}

static uint64 brand (brand_t *p) {
  uint64 hi=p->hi, lo=p->lo, i=p->ind, ret;

  ret = p->tab[i];

  _BR_64STEP_(hi,lo,45,118);

  p->tab[i] = ret + hi;

  p->hi  = hi;
  p->lo  = lo;
  p->ind = hi & _maskr(_BR_LG_TABSZ_);

  return ret;
}

static void brand_init (brand_t *p, uint64 val)

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

/* init / end subroutines */

/* prints information, initializes PRNG, returns number of iterations */

#define INIT_ST  "INIT>"
#define END_ST   "END>"
#define MAX_HOST 80L

#if defined(__GNUC__)
static int64 bench_init (int argc, char *argv[], brand_t *br,
			 timer *t, char *more_args) __attribute__((unused));
#endif
static int64 bench_init (int argc, char *argv[], brand_t *br,
			 timer *t, char *more_args)

{
  uint64 seed;
  int64 niters;
  int i;
  time_t c;
  static char host[MAX_HOST];

  if ((i = sizeof(void *)) != 8) {
    printf ("error: sizeof(void *) = %d\n", i);
    exit(1);
  }
  if ((i = sizeof(long)) != 8) {
    printf ("error: sizeof(long) = %d\n", i);
    exit(1);
  }
  if ((i = sizeof(int)) != 4) {
    printf ("error: sizeof(int) = %d\n", i);
    exit(1);
  }
  
  if (argc < 3) {
    /* prog seed iters [... other args] */
    printf ("Usage:\t%s seed iters %s\n",
	    argv[0], (more_args != NULL) ? more_args : "");
    exit(0);
  }

  printf ("\n======================================================================\n\n");
  
  /* print start time of day */
  time (&c);
  printf ("%s %s started at:  %s", INIT_ST, argv[0], ctime(&c));
  t->init_time = c;

  gethostname (host, MAX_HOST);
  printf ("%s host machine is %s\n", INIT_ST, host);
  
  printf ("%s program built on %s @ %s\n",
	  INIT_ST,  __DATE__, __TIME__);

  seed   = atol (argv[1]);
  niters = atol (argv[2]);

  printf ("%s seed is %ld   niters is %ld\n", INIT_ST, seed, niters);
  if (argc > 3) {
    printf ("%s other args: ", INIT_ST);
    argv += 3;
    while (*argv)
      printf (" %s", *argv++);
    printf ("\n");
  }

  if (br != NULL)
    brand_init (br, seed);

  if (t != NULL)
    timer_clear (t);

  printf ("\n");

  return niters;
}

#if defined(__GNUC__)
static void bench_end (timer *t, int64 iters, char *work)
  __attribute__((unused));
#endif
static void bench_end (timer *t, int64 iters, char *work)

{
  time_t c;
  double wall, cpus, rate;

  printf ("\n");
  
  /* print end time of day */
  time (&c);
  printf ("%s ended at:  %s", END_ST, ctime(&c));
  c = c - t->init_time;
  printf ("%s elapsed time is %d seconds\n", END_ST, (int)c);

  if (t != NULL) {
    timer_report(t, &wall, &cpus, 0);

    printf ("%s %7.3f secs of wall time      ",
	    END_ST, wall);
    if (c <= 0) c = 1;
    printf ("%7.3f%% of value reported by time()\n", wall/c*100.);
	    
    if (wall <= 0)  wall = 0.0001;
    printf ("%s %7.3f secs of cpu+sys time   utilization = %5.3f%%\n",
	    END_ST, cpus, cpus/wall*100.);

    if (cpus > (wall+.01))
      printf ("this result is suspicious since cpu+system > wall\n");
    if ((iters > 0) && (work != NULL)) {
      const char *units[4] = {"", "K", "M", "G"};
      int i = 0;
      
      rate = iters/wall;
      while (i < 3) {
	if (rate > 999.999) {
	  rate /= 1024.;
	  i++;
	}
	else
	  break;
      }
      
      printf ("%s %8.4f %s %s per second\n",
	      END_ST, rate, units[i], work);
    }
  }
}

#endif
