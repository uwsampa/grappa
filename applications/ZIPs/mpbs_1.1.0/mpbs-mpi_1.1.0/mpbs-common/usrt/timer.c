/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: timer.c,v 0.3 2008/03/17 18:31:51 fmmaley Exp fmmaley $ */

// Does some basic benchmarking of usortx.

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <micro64.h>
#include <bitops.h>
#include "usort.h"


#define ACTIMER_MODULE_NAME main
#define ACTIMER_MAIN 1
#include <actimer.h>
enum main_activities {
  a_init, a_rand, a_sort, a_test,
};

// #ifndef _rtc
// #define _rtc actimer_rtc
// #endif

#ifdef LONGIS32
#define STRTOUINT64 strtoull
#define P64 "ll"
#else
#define STRTOUINT64 strtoul
#define P64 "l"
#endif

#if !defined(_rtc)
#if (defined(__ia64) || defined(__ia64__))
#define _rtc get_cycles
static INLINE uint64
get_cycles(void) {
  return __getReg(_IA64_REG_AR_ITC);
}
#elif defined(__x86_64) || defined(__x86_64__)
// how do we get _rtc on Sun Opteron?
// #define _rtc() 0
#endif
#endif

// sort up to 2^M words
#define M 25

static brand_t br;

static void
randinit(void) {
  struct timeval bigtime;
  gettimeofday(&bigtime, NULL);
  brand_init(&br, *((uint64*)(&bigtime)));
}

static void
testsort(int n, uint64* p, uint64 m, int w) {
  int i, fail = 0;
  if (w == 0) {
    uint32* pp = (uint32*)p;
    for (i = 1; i < 2*n && !fail; i++)
      fail |= (pp[i-1] & m) > (pp[i] & m);
  } else {
    //printf("%016lx\n", p[0] & m);
    for (i = w; i < n * w && !fail; i += w) {
      //printf("%016lx\n", p[i] & m);
      fail |= (p[i-w] & m) > (p[i] & m);
    }
  }
  if (fail) {
    printf("Sort FAILED at i=%d\n", i-w);
    exit(1);
  }
}

static double
seconds(void) {
  struct rusage ru;
  getrusage(RUSAGE_SELF,&ru);
  return (ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
         (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) * 1.0e-6;
  // struct timeval bigtime;
  // gettimeofday(&bigtime, NULL);
  // return (bigtime.tv_sec + 1.0e-6 * bigtime.tv_usec);
}

static void
usage(void) {
  fprintf(stderr, "usage: timer [-k] factor [#words/item [scratch-space [mask]]]\n"
	  "1<=factor<2; scratch space is megabytes; mask should be in hex\n");
  exit(1);
}

//static void reset_timers(void);
//static int64 usort_timer_mask(void);

int
main(int argc, char **argv) {
  uint64 *p, *q, mask;
  int i, k, n, bign, s, t, w;
  size_t scratch;
  uint64 time, total, kept;
  double factor, cpu, mb;
  bool kill = false;

  actimer_set_subtlety(1);
  actimer_initialize();
  randinit();
  // Grab command-line arguments
  if (argc < 2)
    usage();
  if (0 == strcmp(argv[1], "-k")) {
    kill = true;
    argc--, argv++;
  }
  factor = atof(argv[1]);
  if (factor < 1.0 || factor >= 2.0) {
    printf("%s: bad size factor: %g\n", argv[0], factor);
    usage();
  }
  w = (argc >= 3) ? atoi(argv[2]) : 1;
  if (w < 1 || w > (1<<12)) {
    printf("%s: bad item size\n", argv[0]);
    usage();
  }
  mb = (argc >= 4) ? atof(argv[3]) : 1;
  if (mb < 0.0) mb = 0.0;
  mask = (argc >= 5) ? STRTOUINT64(argv[4], NULL, 16) : ~UI64(0);
  if (mask == 0) {
    printf("%s: mask must be nonzero\n", argv[0]);
    usage();
  }
  printf("Sorting %d-word items under mask %" P64
	 "x with %g MB scratch space\n", w, mask, mb);
  scratch = mb * 1024.0 * 1024.0;

  // Allocate memory
  bign = (1<<M)*factor;
  actimer_switch(a_init, 0, bign);
  p = malloc(sizeof(uint64) * (bign + (1<<12)));
  q = malloc(2 * sizeof(uint64) * bign);
  if (p == NULL || q == NULL) {
    printf("out of memory\n");
    return 1;
  }
  for (i = 0; i < bign; i++)
    q[i] = i;

  for (k = 16; k <= M; k++) {
    // Compute the number of words per array
    n = (1<<k) * factor;
    kept = 0;

    // Fill memory with random junk
    actimer_switch(a_rand, 0, bign);
    for (i = 0; i < bign + (1<<12); i++)
      p[i] = brand(&br);

    // Do the sort, and time it
    actimer_switch(a_sort, 0, bign);
    t = total = 0;
    cpu = seconds();
    s = n / w;
    for (i = 0; i < (1<<M-k); i++) {
      time = _rtc();
      kept += usortx(p + i*n, s, w * sizeof(uint64), mask, q, scratch,
		     USORT_UNIFORM | (kill ? USORT_KILLUNIQUES : 0));
      //qksort(p+i*n, s, 0L, 63L);
      time = _rtc() - time;
#if defined(__alpha) && defined(__DECC)
      total += (time < 0 ? I64(1)<<32 : 0);
#endif
      total += time;
      t += s;
    }
    cpu = seconds() - cpu;
    // printf("mask: %lx\n", usort_timer_mask());
    // reset_timers();

    // Check the results
    if (!kill) {
      actimer_switch(a_test, 0, bign);
      for (i = 0; i < 1/*(1<<M-k)*/; i++)
	testsort(s, p+(i<<k), mask, w);
    }
    printf("%.3f * 2^%d words: %.2f ticks (%.2f nsec) each", factor, k,
	   (double)total / (t * w), cpu * 1.0e9 / (t * w));
    if (kill)
      printf(" (kept %.4f)", kept / (double)t);
    printf("\n");
    fflush(stdout);
  }

  actimer_summarize(stdout);
  return 0;
}

#if 0
// hack for testing usort timers
#undef ACTIMER_MODULE_NAME
#define ACTIMER_MODULE_NAME usort
#undef ACTIMER_SHARED
#define ACTIMER_SHARED
#undef ACTIMER_MAIN
#include <actimer.h>

static int64
usort_timer_mask(void) {
  return actimer_get_all(0, NULL, NULL);
}

static void
reset_timers(void) {
  actimer_release();
}
#endif


/* $Log: timer.c,v $
 * Revision 0.3  2008/03/17 18:31:51  fmmaley
 * Added -k option for timing USORT_KILLUNIQUES
 *
 * Revision 0.2  2006/08/25 21:25:05  fmmaley
 * Improved version that uses brand() for fast data generation and
 * actimer.h for timing, and adds command-line options for scratch space
 * and sort mask.
 *
 * Revision 0.1  2005/12/30 01:09:35  fmmaley
 *
 */
