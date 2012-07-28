/* UNCLASSIFIED//FOR OFFICIAL USE ONLY */

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: tester.c,v 0.3 2008/03/17 18:29:33 fmmaley Exp fmmaley $ */

// Tests the sort USORTX on both uniform and nonuniform data

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include <ccropt.h>
#include <bitops.h>
#include "usort.h"

#define ACTIMER_MAIN
#define ACTIMER_MODULE_NAME tester
#include <actimer.h>

// Functions for reading and directives for writing 64-bit integers
#ifdef LONGIS32
#define STRTOINT64 strtoll
#define STRTOUINT64 strtoull
#define D "lld"
#define X "llx"
#else
#define STRTOINT64 strtol
#define STRTOUINT64 strtoul
#define D "ld"
#define X "lx"
#endif

// Fills an array with pseudorandom bits
static uint64
scribble(int64 n, int64 stride, uint64 *x, uint64 seed) {
  int64 i, j;
  uint64 sr = seed;
#define MULT UI64(0x5851F42D4C957F2D)
#define FEED UI64(0x6000000000000001)
  for (i = j = 0; i < n; i++, j+=stride) {
    seed = MULT * (seed + 1);
    sr = (sr << 1) | (_popcnt(sr & FEED) & 1);
    x[j] = seed ^ sr;
  }
  return seed;
}

// Computes a checksum of the array x[] of n w-word items (32-bit
// items if w==0).  Uses only the first word of each item.
static uint64
checksum(int64 n, uint64* x, int64 w) {
  uint64 sum = 0;
  int64 i;
  if (w == 0) {
    uint32* y = (uint32*)x;
    for (i = 0; i < n; i++)
      sum += y[i];
  } else {
    for (i = 0; i < n*w; i += w)
      sum += x[i];
  }
  return sum;
}

// Checks whether an array x[] of n items is sorted under a mask.
// The array contains 32-bit items if w==0, or w*64-bit items if w>0.
// If w>1, each item should comprise w identical words.
static int64
notsorted(int64 n, uint64 *x, int64 w, uint64 mask) {
  int64 i, j;
  if (w == 0) {
    uint32* y = (uint32*)x;
    for (i = 1; i < n; i++)
      if ((y[i-1] & mask) > (y[i] & mask))
	return i;
  } else {
    for (i = w; i < n*w; i+=w)
      if ((x[i-w] & mask) > (x[i] & mask))
	return i;
    for (i = 0; i < n*w; i+=w)
      for (j = 1; j < w; j++)
	if (x[i+j] != x[i])
	  return i+j;
  }
  return 0;
}

// Describe the sorting failure at position j of array x
static void
complain(uint64* x, int64 w, int64 j)
{
  if (w == 0) {
    uint32* xx = (uint32*)x;
    printf("usortx halfword sort failed!\n");
    printf("xx[%"D"] = %.8"X"\n", j-1, (uint64)xx[j-1]);
    printf("xx[%"D"] = %.8"X"\n", j, (uint64)xx[j]);
  } else if (j % w == 0) {
    printf("usortx failed to put keys in order!\n");
    printf("x[%"D"] = %.16"X"\n", j-w, x[j-w]);
    printf("x[%"D"] = %.16"X"\n", j, x[j]);
  } else {
    printf("usortx fragmented an item!\n");
    printf("x[%"D"] = %.16"X"\n", j-1, x[j-1]);
    printf("x[%"D"] = %.16"X"\n", j, x[j]);
  }
  exit(1);
}


#define NTEST 8000
#define ORDER(a,b) if(a>b){t=a;a=b;b=t;}
// Runs NTEST tests of USORT on pseudorandom data
int
main(int argc, char *argv[]) {
  int64 a, b, c, i, j, k, m, n, s, ss, t, w, z;
  int64 lb, ub, nn, ww, need, test;
  uint64 before, after, junk, mask, seed, stuff[5], *x, *y, *xcopy;
  bool kill;

  seed = (argc > 1) ? STRTOUINT64(argv[1],NULL,16) : 1;
  printf("seed = %"X"\n", seed);
  for (test = 0; test < NTEST; test++) {
    printf("seed = %"X"\n", seed);
    /* CHOOSE PROBLEM PARAMETERS PSEUDORANDOMLY */
    seed = scribble(5L, 1, stuff, seed);
    junk = stuff[0];
    // data size is random from 4 bits to 22 bits
    s = 4 + (junk % 19);
    n = (junk >> 64-s) + 1;
    // radix is no longer variable
    // r = 3 + ((junk >> 5) % 6);
    // lower and upper bounds are 6-bit random numbers;
    // upper bound is no longer used
    lb = (junk >> 8) & 63;
    ub = (junk >> 14) & 63;
    ORDER(lb,ub);
    mask = (stuff[1] | stuff[2] | stuff[3] | stuff[4]) << lb;
    if (mask == 0)
      continue;
    // choose one bit to nail down, below upper bound
    b = (junk >> 20) & 63;
    c = (junk >> 26) & 1;
    // choose number of words per item
    w = ((junk >> 27) & 15) % 5;
    if (w == 0) {
      mask >>= 32;
      lb = (lb > 32 ? lb-32 : 0);
      nn = (n+1)/2; ww = 1;
    } else {
      n = (n + w - 1) / w;
      nn = n; ww = w;
    }
    // decide how much scratch space to use
    ss = I64(1) << ((junk >> 31) % (s + 5));
    // compute amount to right-shift index bits
    a = (lb < s) ? s-lb : 0;
    m = (UI64(1) << s-a) - 1;
    // will we be able to test killsort?
    j = _leadz(mask);
    kill = !(mask >> b & 1) &&
      _leadz(mask ^ (~UI64(0) >> j)) - j >= 63 - _leadz(n);

    /* GENERATE PSEUDORANDOM DATA */
    x = malloc(nn * ww * sizeof(uint64));
    xcopy = kill ? malloc(nn * ww * sizeof(uint64)) : NULL;
    if (x == NULL || (kill && xcopy == NULL)) {
      printf("unable to allocate %"D" words\n", nn * ww);
      exit(1);
    }
    seed = scribble(nn, ww, x, seed);
    for (j = 0; j < nn * ww; j += ww) {
      // clobber one bit to allow nonuniformity
      t = (x[j] &~ (UI64(1) << b)) | (c << b);
      // add index to help test stability
      t = (t & ~m) | (j >> a);
      for (k = 0; k < ww; k++)
	x[j+k] = t;
    }
    if (kill)
      memcpy(xcopy, x, nn * ww * sizeof(uint64));

    /* ALLOCATE SCRATCH SPACE */
    if (ss >= 4) {
      y = malloc(ss);
      if (y == NULL) {
	printf("unable to allocate %"D" words\n", ss);
	exit(1);
      }
    } else {
      y = NULL;
      ss = 0;
    }

    /* CALL USORT AND TEST THE RESULT */
    before = checksum(n, x, w);
    printf("usortx(x,n=%"D",m=%.16"X",w=%"D"), s=%"D", x<%"D">=%"D", ",
	   n, mask, w, s, b, c);
    if (y != NULL)
      printf("space=%"D"\n", ss);
    else
      printf("space=*\n");
    z = (w==0 ? 4 : w<<3);
    if (y != NULL)
      need = usortx(NULL, n, z, mask, y, ss, USORT_UNIFORM);
    j = usortx(x, n, z, mask, y, ss, USORT_UNIFORM);
    if (j < 0) {
      printf("usortx failed with error code %"D"\n", j);
      exit(1);
    } else if (j > 0 && (y == NULL || ss >= need)) {
      printf("usortx failed to sort stably\n");
      if (y != NULL)
	printf("needed %"D" bytes and had %"D"\n", need, ss);
      exit(1);
    }
    m = (w > 0 && j == 0 ? m : 0) | mask;
    j = notsorted(n, x, w, m);
    if (j != 0)
      complain(x, w, j);
    after = checksum(n, x, w);
    if (before != after) {
      printf("usortx changed the checksum!\n");
      exit(1);
    }

    // If possible, test killsort
    if (kill) {
      int64 nleft = usortx(xcopy, n, z, mask, y, ss,
			   USORT_UNIFORM | USORT_KILLUNIQUES);
      printf("killsort kept %"D" (%.2f%%)\n", nleft, 100 * nleft / (double)n);
      if (j != 0)
	complain(xcopy, w, j);
      // check that all duplicates have survived
      if (w > 0)
	for (i = j = 0; i < n*w; i+=w)
	  if ((i > 0 && (x[i] & mask) == (x[i-w] & mask)) ||
	      (i < n-1 && (x[i] & mask) == (x[i+w] & mask))) {
	    // back up to the first possible match, then search forward
	    while (j > 0 && (xcopy[j-w] & mask) == (xcopy[j] & mask))
	      j -= w;
	    while (j < nleft*w && xcopy[j] != x[i])
	      j += w;
	    if (j >= nleft*w) {
	      printf("item at word #%"D" was lost!\n", i);
	      exit(1);
	    }
	  }
    }

    /* RELEASE MEMORY */
    if (y != NULL) free(y);
    if (kill) free(xcopy);
    free(x);
  }
  printf("usortx passed all tests\n");
  return 0;
}

/* $Log: tester.c,v $
 * Revision 0.3  2008/03/17 18:29:33  fmmaley
 * Added testing of USORT_KILLUNIQUES
 *
 * Revision 0.2  2006/08/25 21:26:42  fmmaley
 * Now varies the mask and the amount of scratch space, in addition to the
 * other parameters.
 *
 * Revision 0.1  2005/12/30 01:08:01  fmmaley
 * *** empty log message ***
 *
 */
