/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: usort.c,v 0.5 2008/03/17 21:49:18 fmmaley Exp fmmaley $ */

// See the file "usort.h" for API details.

#define DEBUG 0   // print debugging info?
#define WARN 0    // warn when data is not uniform?

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bitops.h>
#include <inttypes.h>
#include "usort.h"
#include "ipsort_x.h"
#include "uhash_x.h"

#if ! DEBUG
#define NDEBUG
#endif
#include <assert.h>

// Timers and activities:
// #define ACTIMER_DISABLE
#define ACTIMER_MODULE_NAME usort
#include <actimer.h>
enum usort_activities {
  a_ipsort, a_qpart, a_rxpart, a_killuniq,
  a_firstcount, a_sortlast, a_sortcount, a_scatter,
  a_copy, a_repack, a_repacksort,
  a_overhead,
};

// Check the cache size and define it conservatively if not given
#ifdef CACHE_MB
#if CACHE_MB < 1
#error "CACHE_MB must be a positive integer"
#endif
#else
#define CACHE_MB 1
#endif

// Define prefetch and radix parameters, which are system-dependent.
// Note: we need prefetch to work on 4-byte aligned quantities.
#define LINE 8
#define SHORT I64(24)
#define MINHB 10
#define MAXHB 20

#if defined(__alpha) && defined(__DECC)
//#define _prefetch(a) asm("ldl $31,(%a0)", a)
#define BESTR 6
#define BESTRBIG 8
#define BESTRX 6
#elif (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
//#define _prefetch(a) __lfetch(__lfhint_none,a)
#define BESTR 7
#define BESTRX 5
#elif (defined(__GNUC__) || defined(__INTEL_COMPILER)) && (defined(__x86_64) || defined(__x86_64__))
//#define _prefetch(a)  __asm__ __volatile__ ("prefetch (%0)" : : "r"(a))
#define BESTR 5
#define BESTRX 5
#else
//#define _prefetch(a)
#define BESTR 6
#define BESTRX 5
#endif

#ifndef BESTRBIG
#define BESTRBIG BESTR
#endif

// Define prestash, currently available only on Alpha EV6/EV7
#if defined(__alpha) && defined(__DECC)
#define _prestash(p) asm("wh64 (%a0)", p)
#define PRESTASHING
//#elif (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
//#define _prestash(p) __lfetch_fault_excl(__lfhint_nta,a)
#else
#define _prestash(p)
#endif

#if !defined(_rtc)
#define _rtc() 0
#endif
#define MAX(a,b) ((a)<(b) ? (b) : (a))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#ifndef _CRAY
#define MASK (sizeof(uint64)*LINE-1)
#define ALINE(p) (uint64*)(((intptr_t)(p)+MASK) &~ MASK)
#else
#define ALINE(p) (p)
#endif
// Macro to test whether two arrays of n bytes overlap
#define OVERLAP(p,q,n) ((char*)(q)+(n)>(char*)(p) && (char*)(q)<(char*)(p)+(n))


/* Directives for writing 64-bit integers */
#ifdef LONGIS32
#define D "lld"
#define U "llu"
#define X "llx"
#else
#define D "ld"
#define U "lu"
#define X "lx"
#endif


/*** IN-PLACE SORTING METHODS ***/

// Counts the items x[0:n-1] by pocket (bit positions lb:lb+r-1),
// storing the counts in c[0:2^r-1].  Each item occupies w 64-words
// unless w==0, in which case they are 32-bit items.
static void
firstcount(void* x, int64 n, int64 w, int64 lb, int64 r, uint64 c[])
{
  uint64 m = (UI64(1) << r) - 1;
  int64 i;

  actimer_start(a_firstcount, 1);
  for (i = 0; i <= m; i++)
    c[i] = 0;
  if (w > 0) {
    uint64* xx = (uint64*)x;
#if defined(__INTEL_COMPILER)
    /* these following pragmas don't seem to make a difference:
#pragma noprefetch c
#pragma memref_control c:l1:l1_latency
#pragma loop count (1000000)
    */
#pragma prefetch xx
#pragma memref_control xx:l2:mem_latency
#endif
    for (i = 0; i < n; i++, xx += w) {
#if !defined(__INTEL_COMPILER)
      _prefetch(xx + 4*LINE);
#endif
      c[*xx>>lb & m]++;
    }
  } else
    for (i = 0; i < n; i++)
      c[((uint32*)x)[i]>>lb & m]++;
  actimer_stop(a_firstcount, 1, n);
}


// Performs in-place sorting under the mask m, efficiently if no element
// has to move very far; optionally moves results to z[], which may
// overlap x[]
static void
ipsort(void* x, int64 n, int64 w, uint64 m, void* z) {
#if DEBUG
  printf("IPSORT(x=%p,n=%"D",w=%"D",m=%"X")\n", x, n, w, m);
#endif
  actimer_start(a_ipsort, 1);
  switch (w) {
  case 0: isort_shw((shw*)x, n, m); break;
  case 1: isort_s1w((s1w*)x, n, m); break;
  case 2: isort_s2w((s2w*)x, n, m); break;
  case 3: isort_s3w((s3w*)x, n, m); break;
  default: // unoptimized insertion sort for large items
    {
      uint64 *p = (uint64*)x, *q, *r;
      int64 i, j;
      for (i = 0; i < n; i++, p += w) {
	for (q = p; q > (uint64*)x && (q[-w] & m) > (p[0] & m); q -= w)
	  ;
	//printf("%016lx moves to position %ld\n", p[0], (q - (uint64*)x) / w);
	if (q < p) {
	  for (j = 0; j < w; j++) {
	    uint64 t = p[j];
	    //printf("j=%ld, p=%p, q=%p\n", j, p, q);
	    // the following for statement generates incorrect code with icc!!
	    // the entire loop gets optimized away!!
	    // for (r = p; r > q; r -= w) {
	    r = p;
	    while (r > q) {
	      //printf("p=%p, r=%p\n", p, r);
	      r[j] = r[j-w];
	      r = r - w;
	    }
	    q[j] = t;
	  }
	}
      }
      break;
    }
  }
  if (z != x)
    memmove(z, x, n * (w==0 ? 4 : w<<3));
  actimer_stop(a_ipsort, 1, n);
}

// Performs one pass of in-place radix-exchange sorting on bits
// lb:lb+r-1, and returns the counts c[0:2^r-1].  Assumes, among
// other things, that 1 <= r <= MAXRX.
static void
rxpart(void* x, int64 n, int64 w, int64 lb, int64 r, uint64 c[])
{
#if DEBUG
  printf("RXPART(x=%p,n=%"D",w=%"D",lb=%"D",r=%"D")\n",
	 x, n, w, lb, r);
#endif
  if (r == 1 && w <= 3) {
    int64 i = 0;
    uint64 m = UI64(1) << lb;
    actimer_start(a_qpart, 1);
    switch (w) {
    case 0: i = qpart_shw((shw*)x, n, m); break;
    case 1: i = qpart_s1w((s1w*)x, n, m); break;
    case 2: i = qpart_s2w((s2w*)x, n, m); break;
    case 3: i = qpart_s3w((s3w*)x, n, m); break;
    }
    c[0] = i, c[1] = n-i;
    actimer_stop(a_qpart, 1, n);
  } else {
    uint64 m = (UI64(1) << r) - 1;
    firstcount(x, n, w, lb, r, c);
    actimer_start(a_rxpart, 1);
    switch (w) {
    case 0: rxpart_shw((shw*)x, c, lb, m); break;
    case 1: rxpart_s1w((s1w*)x, c, lb, m); break;
    case 2: rxpart_s2w((s2w*)x, c, lb, m); break;
    case 3: rxpart_s3w((s3w*)x, c, lb, m); break;
    default: // unoptimized radix-exchange for large items
      {
	uint64 *a[1<<MAXRX], *b[1<<MAXRX], *p, *q, t;
	int64 i, j, k;
	// convert counters to pointers
	a[0] = p = x;
	for (i = 0; i < m; i++)
	  b[i] = a[i+1] = (p += w * c[i]);
	b[m] = p + c[m];
	// do the exchange using swaps
	for (i = 0; i <= m; i++) {
	  while (a[i] < b[i]) {
	    q = a[i];
	    _prefetch(q + LINE + w);
	    while ((k = *q>>lb & m) != i) {
	      p = a[k];
	      _prefetch(p + LINE + w);
	      a[k] = p + w;
	      for (j = 0; j < w; j++)
		t = p[j], p[j] = q[j], q[j] = t;
	    }
	    a[i] = q + w;
	  }
	}
	break;
      }
    }
    actimer_stop(a_rxpart, 1, n);
  }
}


/*** NUSORT: RADIX SORT FOR NON-UNIFORM DATA ***/

// Sorts the items x0[0:n-1] into y0[0:n-1] by bit positions lb:lb+r-1,
// given counts c[0:2^r-1] of the number of items in each pocket.
// If newr > 0, also counts the items by bit positions lb+r:lb+r+newr-1,
// storing the counts in c[0:2^newr-1].  Uses the array ptr[0:2^(r+1)-1]
// as scratch space.  Optimized originally for the Alpha EV6.
static void
sortcount(int64 n, void* x0, void* y0, int64 r, int64 lb, int64 newr,
	  uint64* c, void** ptr, int64 w)
{
#ifdef PRESTASHING
  uint64 bitbucket[2*LINE-1];
#endif
  void **a, **b;
  uint64 d, j, m, newm, pad, s;
  uint8* z = (uint8*)y0;
  int64 i;

  m = (UI64(1) << r) - 1;
  /* turn counters into pointers */
  a = ptr;
  b = a + (1<<r);
  s = (w > 0) ? 8*w : 4;
  pad = 16*LINE + (w > 0 ? 8*(w-1) : 0);
  for (j = 0; j <= m; j++) {
    d = s * c[j];
    a[j] = z;
    b[j] = z + (d > pad ? d - pad : 0);
    z += d;
  }

  if (newr == 0) {
    actimer_start(a_sortlast, 1);
    /* just sort without counting */
    if (w == 0) {
      uint32 h, *xx = (uint32*)x0;
      for (i = 0; i < n; i++)
	{ h = *(xx++); *((uint32**)a)[h>>lb & m]++ = h; }
    } else {
      uint64 *x = (uint64*)x0, *y;
      switch (w) {
      case 1:
	for (i = 0; i < n; i++)
	  { d = *(x++); *((uint64**)a)[d>>lb & m]++ = d; }
	break;
      case 2:
	for (i = 0; i < n; i++, x+=2) {
	  d = x[0];
	  j = d>>lb & m; y = a[j]; a[j] = y+2;
	  y[0] = d; y[1] = x[1];
	}
	break;
      case 3:
	for (i = 0; i < n; i++, x+=3) {
	  d = x[0];
	  j = d>>lb & m; y = a[j]; a[j] = y+3;
	  y[0] = d; y[1] = x[1]; y[2] = x[2];
	}
	break;
      default:
	for (i = 0; i < n; i++, x+=w) {
	  d = x[0];
	  j = d>>lb & m; y = a[j]; a[j] = y+w;
	  y[0] = d;
	  for (j = 1; j < w; j++)
	    y[j] = x[j];
	}
	break;
      }
    }
    actimer_stop(a_sortlast, 1, n);
    return;
  }

  /* sort and count new pockets */
  actimer_start(a_sortcount, 1);
  newm = (UI64(1) << newr) - 1;
  for (j = 0; j <= newm; j++)
    c[j] = 0;
  if (w == 0) {
    uint32 h, *xx = (uint32*)x0, *yy;
    for (i = 0; i < n-5; i+=6, xx+=6) {
      _prefetch(xx + 48);
      h = xx[0]; c[h>>(lb+r) & newm]++; *(((uint32**)a)[h>>lb & m]++) = h;
      h = xx[1]; c[h>>(lb+r) & newm]++; *(((uint32**)a)[h>>lb & m]++) = h;
      h = xx[2]; c[h>>(lb+r) & newm]++; *(((uint32**)a)[h>>lb & m]++) = h;
      h = xx[3]; c[h>>(lb+r) & newm]++; *(((uint32**)a)[h>>lb & m]++) = h;
      h = xx[4]; c[h>>(lb+r) & newm]++; *(((uint32**)a)[h>>lb & m]++) = h;
      h = xx[5]; c[h>>(lb+r) & newm]++;
      j = h>>lb & m; yy = ((uint32**)a)[j]++; *yy = h;
      _prestash((yy < (uint32*)b[j]) ?
		yy+2*LINE : (uint32*)(bitbucket+LINE-1));
    }
    for (; i < n; i++) {
      h = *(xx++);
      c[h>>(lb+r) & newm]++;
      *((uint32**)a)[h>>lb & m]++ = h;
    }
  } else {
    uint64 *x = (uint64*)x0, *y;
    switch (w) {
    case 1:
      for (i = 0; i < n-2; i+=3, x+=3) {
	_prefetch(x + 24);
	d = x[0]; c[d>>(lb+r) & newm]++; *(((uint64**)a)[d>>lb & m]++) = d;
	d = x[1]; c[d>>(lb+r) & newm]++; *(((uint64**)a)[d>>lb & m]++) = d;
	d = x[2]; c[d>>(lb+r) & newm]++; j = d>>lb & m;
	y = ((uint64**)a)[j]++; *y = d;
	_prestash((y < (uint64*)b[j]) ? y+LINE : bitbucket+LINE-1);
      }
      for (; i < n; i++) {
	d = *(x++);
	c[d>>(lb+r) & newm]++;
	*(((uint64**)a)[d>>lb & m]++) = d;
      }
      break;
    case 2:
      for (i = 0; i < n-1; i+=2, x+=4) {
	_prefetch(x + 24);
	d = x[0]; c[d>>(lb+r) & newm]++; 
	j = d>>lb & m; y = a[j]; a[j] = y+2;
	y[0] = d; y[1] = x[1];
	d = x[2]; c[d>>(lb+r) & newm]++;
	j = d>>lb & m; y = a[j]; a[j] = y+2;
	y[0] = d; y[1] = x[3];
	_prestash((y < (uint64*)b[j]) ? y+(LINE+1) : bitbucket+LINE-1);
      }
      if (i < n) {
	d = x[0];
	c[d>>(lb+r) & newm]++;
	j = d>>lb & m; y = a[j]; a[j] = y+2;
	y[0] = d; y[1] = x[1];
      }
      break;
    case 3:
      for (i = 0; i < n; i++, x+=3) {
	_prefetch(x + 24);
	d = x[0]; c[d>>(lb+r) & newm]++; 
	j = d>>lb & m; y = a[j]; a[j] = y+3;
	y[0] = d; y[1] = x[1]; y[2] = x[2];
	_prestash((y < (uint64*)b[j]) ? y+(LINE+2) : bitbucket+LINE-1);
      }
      break;
    default: /* not optimized */
      for (i = 0; i < n; i++, x+=w) {
	_prefetch(x + 32);
	d = x[0]; c[d>>(lb+r) & newm]++;
	j = d>>lb & m; y = a[j]; a[j] = y+w;
	y[0] = d;
	for (j = 1; j < w; j++)
	  y[j] = x[j];
      }
    }
  }
  actimer_stop(a_sortcount, 1, n);
}

// Sorts the w-word items x[0:n-1] (where w=0 means 32-bit items) under
// the mask m, using a radix sort with at most r0 bits per pass.  The
// the array y[0:n-1] is available as scratch space, and the results are
// placed in z[0:n-1].  Obviously x[] and y[] must be disjoint.  We
// allow z[] to coincide or overlap with either x[] or y[], but it must
// not overlap both.  Parameters have been checked for validity: n is
// positive, m is nonzero, r0 is between 1 and MAXR inclusive, y is
// non-NULL, etc.  Items are not expected to be uniformly distributed.
//
// There is no penalty for sorting out of place, but an extra copy
// operation is required if z[] shares space with x[] (resp. y[]) and
// the number of phases is odd (resp. even).
static int64
nonuniformsort(void* x, int64 n, int64 w, uint64 m, void* y, int64 r0,
	       void* z)
{
  uint64 c[1<<MAXR];
  int64 i, lb, r;
  void *swap, *tmp[2<<MAXR];

#if DEBUG
  printf("NUSORT(%"X",%"D",%"D",%"X",%"X",%"D")\n",
	 (uint64)x, n, w, m, (uint64)y, r0);
#endif

  while (m != 0) {
    int64 d, ub, phase, phases, full;

    // Divide the current mask interval into phases
    lb = _trailz(m);
    ub = _trailz(m ^ (~UI64(0) << lb));
    d = ub - lb;
    if (d <= r0)
      { r = d; phases = 1; full = 1; }
    else {
      phases = (d+r0-1) / r0;
      r = (d+phases-1) / phases;
      full = d % phases;
      if (full == 0) full = phases;
    }

    // Execute the phases, swapping source and scratch arrays
    firstcount(x, n, w, lb, r, c);
    for (phase = 0; phase < phases; phase++) {
      int64 s = (phase+1 == phases) ? 0 : r - (phase+1 == full);
      // Eliminate the final copy if possible
      if (phase+1 == phases && !OVERLAP(x,z,(w==0 ? 4 : w<<3)*n))
	y = z;
#if DEBUG
      printf("Sorting into 2^%"D" pockets, counting 2^%"D" pockets\n", r, s);
      printf("lb=%"D", ub=%"D", mask=%"X"\n", lb, ub, m);
#endif
      sortcount(n, x, y, r, lb, s, c, tmp, w);
      swap = x, x = y, y = swap; 
      lb += r; r = s;
    }
    m &= ~_maskr(ub);
  }

  // Now x points to the results.  Copy them to z[] if necessary.
  if (x != z) {
    actimer_start(a_copy, 1);
    if (w > 0)
      for (i = 0; i < n*w; i++)
	((uint64*)z)[i] = ((uint64*)x)[i];
    else
      for (i = 0; i < n; i++)
	((uint32*)z)[i] = ((uint32*)x)[i];
    actimer_stop(a_copy, 1, n);
  }

  return I64(0);
}


/*** USORT: RADIX SORT FOR UNIFORM DATA ***/

/* Pack items from 2^r pockets into the array dest[]; source[i] points
 * to the ith pocket and source[i+2^r] points just beyond its end.
 * The number of words per item is only used to determine whether the
 * items are halfwords.  The halfword code could be optimized using
 * memmove() or 64-bit loads and stores, but it's only called when the
 * data turned out not to be uniform.  The source and destination
 * arrays can overlap, but if so, then dest[k*w] is at a lower address
 * than the kth source item for every k, so sequential copying in the
 * natural order will work. */
static void
repack(int64 r, void** source, void* dest, int64 w, int64 n) {
  int64 i;

  actimer_start(a_repack, 1);
  if (w > 0) {
    uint64 *p, *q, *t = (uint64*)dest;
    for (i = 0; i < I64(1)<<r; i++) {
      p = (uint64*)source[i];
      q = (uint64*)source[(I64(1)<<r)+i];
      for (; p < q; p++, t++)
	*t = *p;
    }
  } else {
    uint32 *pp, *qq, *tt = (uint32*)dest;
    for (i = 0; i < I64(1)<<r; i++) {
      pp = (uint32*)source[i];
      qq = (uint32*)source[(I64(1)<<r)+i];
      for (; pp < qq; pp++, tt++)
	*tt = *pp;
    }
  }
  actimer_stop(a_repack, 1, n);
}

/* Pack items from 2^r pockets into the array dest[], and perform
 * an insertion sort, using the mask m to restrict the sort keys.
 * Again source[i] points to the ith pocket and source[i+2^r]
 * points just beyond its end.  Tricky point: some of the source
 * pockets can share space with the destination.  If they do, then
 * the nth source item must be at dest[n+w-1] or higher. */

// extern void ia64_isort(uint64 * restrict s, uint64 * restrict t,
//                        int64 n, uint64 m);

static void
repacksort(int64 r, void** source, void* dest, uint64 m,
	   int64 w, int64 n) {
  int64 i, j, k, v;

#if DEBUG
  printf("Joining 2^%"D" pockets with insertion sort\n", r);
#endif
  actimer_start(a_repacksort, 1);
  if (w == 0) {
    uint32 *pp, *qq, *tt = (uint32*)dest;
    for (i = 0; i < I64(1)<<r; i++) {
      pp = (uint32*)source[i];
      qq = (uint32*)source[(I64(1)<<r)+i];
      k = qq - pp;
      if (k > 0)
	nisort_shw((shw*)pp, (shw*)tt, k, m);
      tt += k;
    }
  } else {
    uint64 *p, *q, * restrict t, d;
    t = (uint64*) dest;
    for (i = 0; i < I64(1)<<r; i++) {
      p = (uint64*)source[i];
      q = (uint64*)source[(I64(1)<<r)+i];
      k = q - p;
      if (k == 0) continue;
      switch (w) {
      case 1: nisort_uint64(p, t, k, m); break;
      case 2: nisort_s2w((s2w*)p, (s2w*)t, k>>1, m); break;
      case 3: nisort_s3w((s3w*)p, (s3w*)t, k/3, m); break;
      default: // not optimized
	for (k = 0; p < q; p+=w, k+=w) {
	  _prefetch(p + 4*LINE);
	  d = p[0];
	  for (j = k; j > 0 && (t[j-w] & m) > (d & m); j-=w)
	    for (v = 0; v < w; v++)
	      t[j+v] = t[j-w+v];
	  t[j] = d;
	  for (v = 1; v < w; v++)
	    t[j+v] = p[v];
	}
	break;
      }
      t += k;
    }
  }
  actimer_stop(a_repacksort, 1, n);
}

/* The main subroutine: sorts from 2^s pockets to 2^t pockets
 * according to bit positions lb:lb+t-1.  The ith source pocket begins
 * at source[i] and ends just before source[i+2^s].  The jth
 * destination pocket begins at target[j] and has size bs (in 64-bit
 * words), of which the last w * ceiling(LINE/w) words are reserved as
 * padding.  There can be at most one gap in the sequence of
 * destination pockets.  Pockets must be word-aligned, and also
 * cache-aligned if prestashing is in effect.
 *
 * Returns 0 if successful and 1 if some pocket overflowed.  Upon
 * a successful return, target[j+2^t] points to the first word
 * beyond the data stored in the jth pocket. */
static int
scatter(int64 s, uint64** source, int64 t, uint64** target,
	int64 lb, uint64 bs, uint64 w, int64 n) {
  uint64 **a;
  uint64 *b0, *b1, * restrict p, *q, *x;
  uint64 d, k, m, pad;
  int64 i, j;

#if DEBUG
  printf("Sorting from 2^%"D" pockets to 2^%"D" pockets\n", s, t);
#endif
  actimer_start(a_scatter, 1);
  assert(w == 0 || (bs % w) == 0);
  pad = (w == 0 ? LINE : w * ((LINE + w-1) / w));
  /* find barriers b0 and b1 */
  b0 = NULL;
  a = target + (I64(1)<<t);
  a[0] = p = target[0];
  for (j = 1; j < I64(1)<<t; j++) {
    q = target[j];
    if (q != p + bs) {
      assert(b0 == NULL);
      b0 = p + bs - pad;
    }
    a[j] = p = q;
    _prestash(q);
  }
  b1 = p + bs - pad;
#if DEBUG
  printf("b0=%"X", b1=%"X"\n", (uint64)b0, (uint64)b1);
#endif

  m = (UI64(1) << t) - 1;
  for (i = 0; i < I64(1)<<s; i++) {
    uint64 fail = 0;
    p = source[i];
    q = source[(I64(1)<<s)+i];
    /* inner loop optimized separately for each w */
    switch (w) {
    case 0:
      {
	uint32 h, *xx, *pp = (uint32*)p, *qq = (uint32*)q;
	uint32 *bb0 = (uint32*)b0, *bb1 = (uint32*)b1;
	for (; pp < qq-5; pp+=6) {
	  _prefetch(pp + 96);
	  h = pp[0]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  fail |= (xx == bb0) || (xx == bb1);
	  h = pp[1]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  fail |= (xx == bb0) || (xx == bb1);
	  h = pp[2]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  fail |= (xx == bb0) || (xx == bb1);
	  h = pp[3]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  fail |= (xx == bb0) || (xx == bb1);
	  h = pp[4]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  fail |= (xx == bb0) || (xx == bb1);
	  h = pp[5]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  fail |= (xx == bb0) || (xx == bb1);
	  /* explicit loop is a bit slower:
	  for (j = 0; j < 6; j++) {
	    h = pp[j]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	    fail |= (xx == bb0) || (xx == bb1);
	  }
	  */
	  if (fail) goto bailout;
	  _prestash(xx + 2*LINE);
	}
	for (; pp < qq; pp++) {
	  h = pp[0]; xx = ((uint32**)a)[h>>lb & m]++; *xx = h;
	  if (xx == bb0 || xx == bb1) goto bailout;
	}
      }
      break;
    case 1:
#if 0
      for (; p < q-2; p+=3) {
	_prefetch(p + 48);
	d = p[0]; x = a[d>>lb & m]++; *x = d;
	fail |= (x == b0) || (x == b1);
	d = p[1]; x = a[d>>lb & m]++; *x = d;
	fail |= (x == b0) || (x == b1);
	d = p[2]; x = a[d>>lb & m]++; *x = d;
	fail |= (x == b0) || (x == b1);
	if (fail) goto bailout;
	_prestash(x + LINE);
      }
      for (; p < q; p++) {
	d = p[0]; x = a[d>>lb & m]++; *x = d;
	if (x == b0 || x == b1) goto bailout;
      }
#else
      for (; p < q; p++) {
	d = p[0]; x = a[d>>lb & m]++; *x = d;
	if (x == b0 || x == b1) goto bailout;
      }
#endif
      break;
    case 2:
      for (; p < q-3; p+=4) {
	_prefetch(p + 48);
	d = p[0]; k = d>>lb & m; x = a[k]; a[k] = x+2;
	x[0] = d; x[1] = p[1];
	fail |= (x == b0) || (x == b1);
	d = p[2]; k = d>>lb & m; x = a[k]; a[k] = x+2;
	x[0] = d; x[1] = p[3];
	fail |= (x == b0) || (x == b1);
	if (fail) goto bailout;
	_prestash(x + LINE + 1);
      }
      if (p < q) {
	d = p[0]; k = d>>lb & m; x = a[k]; a[k] = x+2;
	x[0] = d; x[1] = p[1];
	if (x == b0 || x == b1) goto bailout;
      }
      break;
    case 3:
      for (; p < q; p+=3) {
	_prefetch(p + 48);
	d = p[0]; k = d>>lb & m; x = a[k]; a[k] = x+3;
	x[0] = d; x[1] = p[1]; x[2] = p[2];
	if (x == b0 || x == b1) goto bailout;
	/* the following is safe under our restrictive assumptions */
	_prestash(x + LINE + 2);
      }
      break;
    default: /* not optimized */
      for (; p < q; p+=w) {
	_prefetch(p + 48);
	d = p[0]; k = d>>lb & m; x = a[k]; a[k] = x+w;
	x[0] = d;
	for (j = 1; j < w; j++)
	  x[j] = p[j];
	if (x == b0 || x == b1) goto bailout;
	_prestash(x + (LINE + w-1));
      }
    }
  }
  actimer_stop(a_scatter, 1, n);

  /* test for overflow */
  for (j = 0; j < I64(1)<<t; j++)
    if (a[j] > target[j] + bs - pad) {
#if DEBUG
      printf("Pocket %"D" overflowed (%"U" > %"U")\n",
	     j, (uint64)(a[j] - target[j]), bs - pad);
#endif
      return 1;
    }
  return 0;

 bailout:
#if DEBUG
  printf("Some pocket hit barrier\n");
#endif
  actimer_stop(a_scatter, 1, 0);
  return 1;
}

// Computes a slight overestimate of the floor of the square
// root of n, using only integer arithmetic and bit counting.
static uint64
oversqrt(uint64 n) {
  uint64 t = 64 - _leadz(n); // 2^(t-1) <= n < 2^t
  t = UI64(1) << t/2;        // initial estimate, within factor of sqrt(2)
  return (t + n/t) / 2;      // within a factor of 3/sqrt(8)
}

// A data structure that expresses a plan for usort.  The plan will
// only be executed if there is enough scratch space available.
typedef struct plan_s {
  bool  kill;   // are we killing singletons?
  int64 radix;  // bits in the optimum radix
  int64 phases; // number of sorting phases
  int64 lb;     // low bit at which to start sorting
  int64 ub;     // 64 - _leadz(mask)
  int64 bs;     // block size (pocket size) in words
  int64 room;   // number of blocks that fit in the original array
  int64 need;   // words of scratch space needed
} plan_t;

// Computes a plan for fast out-of-place radix sorting assuming uniform
// data.  Inputs: n0 = minimum number of items, n1 = maximum number of
// items, w = number of words per item, m = mask under which to sort, r
// = maximum and preferred radix size.  A nonzero return means that no
// plan was found due to holes in the mask, so nusort and ipsort must be
// used instead; the return value is the mask to pass to nusort, and
// then ipsort must be called to finish up.
static uint64
makeplan(int64 n0, int64 n1, int64 w, uint64 m, int64 r, plan_t *plan)
{
  int64 i, p, avg, sigma, bs, blox, room, run, span, unit;

  // Figure out how many bits we would like to sort on.  We would like
  // to leave an average of less than 2 items per category.  If the
  // upper part of the mask is not contiguous, use nusort instead.
  span = MAX(1, 63 - _leadz(n1));
  // FIXME: the 63 should become 62 when a better isort is available
  span = MIN(_popcnt(m), span);
  i = _leadz(m);
  run = _leadz(m ^ (~UI64(0) >> i)) - i;
  if (run < span) {
    // figure out what part of the mask to use
    if (_popcnt(m) <= span)
      return m;
    for (i = 64 - i - run; run < span; --i)
      run += (m >> (i-1)) & 1;
    return m &~ ((UI64(1)<<i) - 1);
  }
  if (plan == NULL)
    return UI64(0);

  // Divide the task into phases and adjust the radix
  p = (span + r-1) / r;
  if (p*r > run) {
    r = (run + p - 1) / p;
    plan->lb = 64 - i - run;
  } else
    plan->lb = 64 - i - p*r;
  plan->ub = 64 - i;
  plan->radix = r;
  plan->phases = p;

  // Compute the block size and the number of words in a "unit": the
  // least common multiple the of item and cache line sizes.  When
  // choosing the block size, we allow the number of items in a bucket
  // to vary by at least 5 standard deviations.  Normally we model this
  // number as Poisson, but killing singletons increases the variance,
  // relative to the mean, by a factor of up to 2.08196.  For simplicity
  // we use a factor of 2 + 21/256 = 2.08203.
  avg = (n1 >> r) + 1;
  sigma = oversqrt(plan->kill ? 2*avg + (21*avg >> 8) : avg);
  if (w > 0) {
    i = _trailz(LINE) - _trailz(w);
    unit = w << MAX(i,0);
    bs = w * (avg + 5 * sigma) + LINE;
  } else {
    unit = LINE;
    bs = (avg + 5 * sigma + 1) / 2 + LINE;
  }

  // Round the block size up to an integral number of units --
  // an odd number if unit is a power of two, to fill the cache better
  if (_popcnt(unit) == 1)
    bs = ((bs + unit-1) &~ (2*unit-1)) + unit;
  else
    bs = ((bs + unit-1) / unit) * unit;
  plan->bs = bs;

  // Compute the scratch space requirement
  if (p > 1) {
    // Decide how many pockets will fit in the x array, accounting
    // for padding at the beginning and cache alignment.  The padding
    // ensures that when repacking items into the original array,
    // no source item partially overlaps its destination.
    room = (w > 0 ? n0 * w : n0 >> 1) - (LINE-1) - (w>1 ? w : 0);
    room = (room > 0) ? ((room / bs) &~ 1) : 0;
    blox = (2 << r) - room;
  } else {
    // If there is only one phase, we don't need pockets in the x array
    room = 0;
    blox = (I64(1) << r);
  }
  plan->room = room;
  plan->need = blox * bs + LINE - (w > 0);
  return UI64(0);
}

// Here is the function that does the real usort work.  It assumes that
// the scratch space y[] is adequate, allowing for cache-alignment.
// Input comes from x[], and output is placed in z[], which may overlap
// x[] provided that z <= x.  The return value is the mask on which
// non-uniform sorting must be done within z[]; if the uniform sort
// succeeded, then this mask will be zero.
//
// There is essentially no penalty for sorting out of place (z != x); it
// only affects the final repacking, which is always required unless the
// first pass discovers nonuniformity.
static uint64
uniformsort(void* x, int64 n, int64 w, uint64 m, void* y, plan_t *plan,
	    void* z)
{
  uint64 counts[4<<MAXR];
  uint64 **source, **target, **work, *p, *q;
  void *bounds[2];
  int64 i, full, low, phase;
  int64 r = plan->radix, lb = plan->lb, ub = plan->ub,
    phases = plan->phases, bs = plan->bs, room = plan->room;
#define SWAP(S,T) (work=S, S=T, T=work)

#if DEBUG
  printf("USORT(%p,%"D",%"D",%"X",%p,%"D",%p)\n",
	 x, n, w, m, y, plan->radix, z);
#endif

  /* STEP 0: setup, with cache alignment and padding */
  full = (plan->ub - plan->lb) % phases;
  if (full == 0) full = phases;
  source = (uint64**)counts;
  target = (uint64**)(counts + (2 << r));
  p = ALINE(y);
  q = ALINE((char*)x + (w>1 ? w<<3 : 0));

  /* STEP 1: perform the first sort into 2^r pockets */
  for (i = 0; i < I64(1)<<r; i++)
    source[i] = p, p += bs;
  bounds[0] = x;
  bounds[1] = (w > 0 ? (void*)((uint64*)x + n * w) : (void*)((uint32*)x + n));
  phase = 0;
  if (scatter(0, (uint64**)bounds, r, source, lb, bs, w, n))
    goto finish;
  lb += r; phase = 1;

  /* STEP 2: repeatedly sort from 2^r pockets to 2^r pockets */
  if (phase < phases) {
    for (i = 0; i < room; i++)
      target[i] = q, q += bs;
    for (; i < I64(1)<<r; i++)
      target[i] = p, p += bs;
    for (; phase < full; phase++, lb += r, SWAP(source,target)) {
      if (scatter(r, source, r, target, lb, bs, w, n))
	goto finish;
    }
  }

  /* STEP 3: sort from 2^r pockets to 2^(r-1) pockets */
  if (phase < phases) {
    /* join up pockets in pairs; they remain composed of units */
    for (i = 1; i < I64(1) << (r-1); i++)
      target[i] = target[2*i];
    bs = 2 * bs;
    if (scatter(r, source, r-1, target, lb, bs, w, n))
      goto finish;
    phase++, lb += r-1, SWAP(source,target);
  }

  /* STEP 4: repeatedly sort from 2^(r-1) into 2^(r-1) pockets */
  if (phase < phases) {
    for (i = 1; i < I64(1) << (r-1); i++)
      target[i] = target[2*i];
    for (; phase < phases; phase++, lb += r-1, SWAP(source,target))
      if (scatter(r-1, source, r-1, target, lb, bs, w, n))
	goto finish;
  }

  /* STEP 5: pack answers into destination array */
 finish:
  low = (_trailz(m) < plan->lb);
  if (low && lb == ub)
    repacksort(r - (phase > full), (void**)source, z, m, w, n);
  else if (phase > 0 || z != x)
    repack(r - (phase > full), (void**)source, z, w, n);

  /* STEP 6: return the mask on which nusorting is required */
#if WARN
  if (lb != ub)
    printf("usort warning: among %"D" keys, bits %"D":%"D" are nonuniform\n",
	   n, lb, lb-1 + r - (phase >= full));
#endif
  return (lb == ub) ? I64(0) : (low ? m : m &~ ((UI64(1)<<lb) - 1));
}


/*** CODE THAT IS COMMON TO UNIFORM AND NON-UNIFORM SORTING ***/

static int64 mainsort(void* x, int64 n, int64 w, uint64 m,
		      void* y, size_t t, int64 r, plan_t *plan, void* z);

// This sorter does a partitioning step in-place and calls back
// to mainsort(), if necessary, to sort the pieces.
static int64
inplacesort(void* x, int64 n, int64 w, uint64 m,
	    void* y, size_t t, int64 r0, plan_t* plan,
	    void* z)
{
  plan_t newplan;
  uint64 c[1<<MAXRX], mm, m0;
  int64 i, lb, r, rmax, nmin, nmax, uniform;
  size_t s = (w == 0) ? 4 : w<<3;

  // Determine how many bits to partition on, and where they are.
  i = _leadz(m);
  rmax = _leadz(m ^ (~UI64(0) >> i)) - i;
  r = (plan != NULL) ? plan->radix : 0;
  if (r == 0) r = rmax;
  r = MIN(BESTRX, r);
  rmax = MIN(MAXRX, rmax);
  // Try to improve efficiency in the endgame: Partition on one more
  // bit if it's allowed and avoids further partitioning.
  r += ((n >> r) >= SHORT && (n >> r) < 2*SHORT && r < rmax);
  assert(r > 0 && r <= MAXRX);
  lb = 64 - i - r;

  // Do the partitioning.
  rxpart(x, n, w, lb, r, c);

  // Finish immediately if possible
  mm = m &~ (((UI64(1)<<r)-1) << lb);
  if (mm == 0) {
    if (z != x)
      memmove(z, x, n * s);
    return I64(1);
  }
  if (plan != NULL && n <= SHORT<<r) {
    ipsort(x, n, w, m, z);
    return I64(1);
  }

  // In the uniform case, check whether things still look uniform and
  // if so, find a good plan for all the subproblems
  uniform = (plan != NULL);
  if (uniform && t > 0) {
    newplan.kill = plan->kill;
    nmin = n, nmax = 0;
    for (i = 0; i < I64(1)<<r; i++)
      nmin = MIN(nmin, c[i]), nmax = MAX(nmax, c[i]);
    uniform = (nmin >= n >> (r+1)) && (nmax <= n >> (r-1));
    if (uniform) {
      m0 = makeplan(nmin, nmax, w, mm, r0, &newplan);
      uniform = (m0 == 0);
    }
  }

  // Now deal with the subintervals
  plan = uniform ? &newplan : NULL;
  for (i = 0; i < I64(1)<<r; i++) {
    void* x0 = x;
    // Do as much short sorting as possible
    n = 0;
    for (; i < I64(1)<<r && c[i] <= SHORT; i++) {
      x = (char*)x + c[i] * s;
      n += c[i];
    }
    if (n > 0) {
      if (n > 1)
	ipsort(x0, n, w, m, z);
      z = (char*)z + n * s;
    }
    // Sort recursively if necessary
    if (i < I64(1)<<r) {
#if !defined(NDEBUG)
      int64 ans =
#endif
	mainsort(x, c[i], w, mm, y, t, r0, plan, z);
      assert(ans >= 0);
      x = (char*)x + c[i] * s;
      z = (char*)z + c[i] * s;
    }
  }
  return I64(1);
}

// The function mainsort() dispatches all sorting requests not involving
// KILLUNIQUES.  It sorts from x[] to z[], using y[] as scratch space.
// It assumes that x is non-NULL, that either y is non-NULL or t is
// zero, and that all other parameters have valid values.  Nonuniform
// keys are signified by a null plan.  The plan can also be blank
// (plan->radix == 0), meaning the keys are uniform and the mask is
// acceptable for uniformsort(), but no plan has been made yet.  Return
// value: 0 if the sort was stable, 1 if unstable.
static int64
mainsort(void* x, int64 n, int64 w, uint64 m,
	 void* y, size_t t, int64 r, plan_t *plan,
	 void* z)
{
  // Figure out whether we have enough scratch space or whether we
  // must do some sorting in place.  FIXME: for small n, always sort in place!
  size_t bulk = n * (w==0 ? 4 : w<<3);
  int64 uniform = (plan != NULL);
  int go = (t >= bulk + uniform);
  if (go && uniform) {
    // In the uniform case, a plan is needed
    if (plan->radix == 0)
      makeplan(n, n, w, m, r, plan);
    go = (t >= sizeof(uint64) * plan->need);
  }

  // Decide how to sort
  if (!go) {
    return inplacesort(x, n, w, m, y, t, r, plan, z);
  } else if (uniform) {
    uint64 m0 = uniformsort(x, n, w, m, y, plan, z);
    // If uniform sort failed, proceed to nonuniform sort.
    // It requires no more scratch space.
    return m0 ? nonuniformsort(z, n, w, m0, y, r, z) : I64(0);
  } else
    return nonuniformsort(x, n, w, m, y, r, z);
}


/*** REMOVING UNMATCHED ITEMS AND SORTING THE REMAINDER ***/

static int64
remove_unmatched(void* x, int64 n, int64 w, int64 lb, int64 h,
		 uint64* ht, void* y)
{
  uint64 m = _maskr(h-6);
  int64 k;

  actimer_start(a_killuniq, 1);
  for (int64 i = 0; i < 1<<(h-6); i++)
    ht[i] = 0;
  switch (w) {
  case 0: k = remunm_shw((shw*)x, n, lb, m, ht, y, (shw*)x); break;
  case 1: k = remunm_s1w((s1w*)x, n, lb, m, ht, y, (s1w*)x); break;
  case 2: k = remunm_s2w((s2w*)x, n, lb, m, ht, y, (s2w*)x); break;
  case 3: k = remunm_s3w((s3w*)x, n, lb, m, ht, y, (s3w*)x); break;
  default:
    {
      uint64 *a = (uint64*)x, *b = (uint64*)y, d;
      int64 i, j;
      k = 0;
      for (i = 0; i < n; i++) {
	b[k] = d = a[i*w];
	d >>= lb;
	if (ht[d>>6 & m] & UI64(1)<<(d&63))
	  k++;
	ht[d>>6 & m] ^= UI64(1)<<(d&63);
      }
      for (i = 0; i < k; i++) {
	d = b[i] >> lb;
	ht[d>>6 & m] &= ~(UI64(1)<<(d&63));
      }
      k = 0, b = a;
      for (i = 0; i < n; i++) {
	d = a[i*w] >> lb;
	if ( !(ht[d>>6 & m] & UI64(1)<<(d&63)) ) {
	  for (j = 0; j < w; j++)
	    *(b++) = a[i*w+j];
	  k++;
	}
      }
    }
    break;
  }
  actimer_stop(a_killuniq, 1, n);
  return k;
}

// Input is x (n w-word items), scratch is y (size t > 0), destination
// is z, preferred radix is r; return value is number of items kept.
// The mask must be contiguous.
static int64
killsort(void* x, int64 n, int64 w, uint64 m,
	 void* y, size_t t, int64 r, void* z)
{
  plan_t splan = { true };

#if DEBUG
  printf("KSORT(%p,%"D",%"D",%"X",%p,%"D",%"D",%p)\n",
	 x, n, w, m, y, t, r, z);
#endif

  // Compute the number of bits to hash on
  int64 logn = 63 - _leadz(n);  // this is floor(log2(n)), or -1 if n==0
  int64 h = MIN(_popcnt(m), logn+4);
  // *** FIXME *** is this the right formula?
  if (logn < MAXHB-1)
    h = MIN(h, MAXHB);

  // If the task is too small, or we won't get enough cutdown, just sort
  if (h < MINHB || _popcnt(m) <= logn) {
    mainsort(x, n, w, m, y, t, r, &splan, z);
    return n;
  }

  // Do we want to hash on this many bits?  And if so, do we have
  // enough scratch space for the hash table and n/2 duplicates?
  size_t s = (w==0 ? 4 : w<<3);
  if (h > MAXHB || t < ((size_t)1 << (h-3)) + (n*s >> 1)) {
    // No, so we must partition the input and operate on the pieces
    uint64 c[1<<MAXRX];
    int64 i, k, lb;
    int64 rx = MIN(BESTRX, r);
    // We have _popcnt(m) >= MINHB > MAXRX >= rx
    assert(_popcnt(m) > rx);
    lb = 64 - _leadz(m) - rx;
    rxpart(x, n, w, lb, rx, c);
    // FIXME: it might be best to make a common plan...
    m = m &~ (_maskr(rx) << lb);
    // Note that m cannot be zero here
    n = 0;
    for (i = 0; i < I64(1)<<rx; i++) {
      int64 ci = c[i];
      k = killsort(x, ci, w, m, y, t, r, z);
      x = (char*)x + ci * s;
      z = (char*)z + k * s;
      n += k;
    }
    return n;
  } else {
    // Do the hashing here, and sort the output
    uint64* ht = (uint64*)y;
    int64 lb = 64 - _leadz(m) - h;
    int64 k = remove_unmatched(x, n, w, lb, h, ht, ht + (1 << (h-6)));

    mainsort(x, k, w, m, y, t, r, &splan, z);
    return k;
  }
}


/*** TOP-LEVEL FUNCTIONS ***/

// Find out which data bits vary, assuming n > 0
static uint64
nonconstant(void* x, int64 n, size_t s) {
  uint64 m = 0;
  int64 i, w;

  if (s == 4) {
    uint32* pp = (uint32*)x;
    uint32 v = pp[0];
    for (i = 1; i < n; i++)
      m |= pp[i] ^ v;
  } else {
    uint64* p = (uint64*)x;
    uint64 v = p[0];
    w = s >> 3;
    for (i = 1, p += w; i < n; i++, p += w)
      m |= *p ^ v;
  }

  return m;
}

// The primary interface to applications:
int64
usortx(void* x, int64 n, size_t s, uint64 m,
       void* y, size_t t, uint64 flags)
{
  // Begin with no plan
  plan_t splan = { false }, *plan;
  size_t tbest;
  uint64 low, newm, *y0 = y;
  int64 ans, r, w;
  bool uniform, unstable;

  // Check and adjust parameters
  if (sizeof(uint64*) > sizeof(uint64) || (s == 4 && sizeof(shw) != 4))
    return I64(-4);
  low = (s == 4) ? 3 : 7;
  if ((x != NULL && ((intptr_t)x & low) != 0) ||
      (y != NULL && ((intptr_t)x & low) != 0))
    return I64(-3);
  if (s == 0 || ((s & 7) != 0 && s != 4))
    return I64(-2);
  if (n < 0 || m == 0 || (s == 4 && m>>32 != 0) ||
      _leadz(n) + _leadz(s) <= 64)
    return I64(-1);
  if (n <= 1)
    return I64(0);
  actimer_start(a_overhead, 1);
  r = (flags & USORT_BIGPAGES) ? BESTRBIG : BESTR;
  uniform = (flags & USORT_UNIFORM) != 0;
  unstable = (flags & (USORT_UNSTABLE | USORT_KILLUNIQUES)) != 0;
  w = s >> 3;

  if (flags & USORT_PAREMASK)
    m &= nonconstant(x, n, s);

  // In the uniform case, figure out as much of the plan as we can.
  // (If our scratch space is given, we can't complete the plan yet,
  // because we might have to do some in-place partitioning first.)
  // Use nusort if the bits we need to sort on are not contiguous.
  splan.kill = (flags & USORT_KILLUNIQUES) != 0;
  plan = (x == NULL || y == NULL) ? &splan : NULL;
  newm = uniform ? makeplan(n, n, w, m, r, plan) : 0;
  if (newm != 0)
    uniform = false;
  else
    newm = m;

  // Prepare scratch space if necessary.  First figure out how much
  // we would need to sort stably, and then reduce it if permitted.
  // For best performance we want to use half the cache for scratch
  // and leave the other half for segments of the original array.
  if (x == NULL || y == NULL) {
    tbest = uniform ? sizeof(uint64) * splan.need : n * s;
    if (unstable || (x != NULL && t > 0 && t < tbest))
      tbest = MIN(tbest, (size_t)(CACHE_MB) << 19);
    if (x == NULL)
      return (int64)tbest;
    // Here y == NULL, and we prefer 'tbest' bytes of scratch space.
    if (t == 0 || t > tbest)
      t = tbest;
    while ((y = malloc(t)) == NULL)
      if ((t >>= 4) < (s << r) + sizeof(uint64)*(LINE-1))
	break;
    if (y == NULL)
      t = 0;
  } else {
    if (unstable)
      t = MIN(t, (size_t)(CACHE_MB) << 19);
  }

  // Execute the sort, with or without a known plan
  if (uniform && (flags & USORT_KILLUNIQUES) && t > 0 &&
      _leadz(m) + _popcnt(m) + _trailz(m) == 64)
    ans = killsort(x, n, w, m, y, t, r, x);
  else {
    plan = (uniform ? &splan : NULL);
    ans = mainsort(x, n, w, newm, y, t, r, plan, x);
    if (m != newm)
      ipsort(x, n, w, m, x);
    if (flags & USORT_KILLUNIQUES)
      ans = n;
  }
  if (y0 == NULL && y != NULL)
    free(y);
  actimer_stop(a_overhead, 1, n);
  return ans;
}

// The simple entry point for usort
int64
usort(uint64* x, int64 n, int64 lb, int64 ub) {
  uint64 m = ((UI64(1) << ub) << 1) - (UI64(1) << lb);
  if (lb < 0 || ub > 63 || ub < lb)
    return I64(-2);
  return usortx(x, n, sizeof(uint64), m, NULL, 0, USORT_UNIFORM);
}

// The simple entry point for nusort
int64
nusort(uint64* x, int64 n, int64 lb, int64 ub) {
  uint64 m = ((UI64(1) << ub) << 1) - (UI64(1) << lb);
  if (lb < 0 || ub > 63 || ub < lb)
    return I64(-2);
  return usortx(x, n, sizeof(uint64), m, NULL, 0, 0);
}

// FIXME: do we want a simple entry point for in-place sorting?


/* FORTRAN INTERFACES */

#if 0
int64
nusort_(uint64* x, int64 *n, int64 *lb, int64 *ub) {
  return nusort(x, *n, *lb, *ub);
}

int64
usort_(uint64* x, int64 *n, int64 *lb, int64 *ub) {
  return usort(x, *n, *lb, *ub);
}

int64
usortx_(void* x, int64 *n, size_t *s, uint64 *m,
	void* y, size_t *t, uint64 *flags) {
  return usortx(x, *n, *s, *m, y, *t, *flags);
}
#endif

/* $Log: usort.c,v $
 * Revision 0.5  2008/03/17 21:49:18  fmmaley
 * Fixed uninitialized variable in remove_unmatched()
 *
 * Revision 0.4  2008/03/17 18:35:47  fmmaley
 * Added support for USORT_PAREMASK and USORT_KILLUNIQUES.  Implementing
 * "killsort" involved giving many internal routines the ability to write
 * their output elsewhere than atop their input.
 *
 * Revision 0.3  2006/08/25 21:10:11  fmmaley
 * Replaced timing code with actimer; removed prefetch definitions now in
 * bitops.h; added special-case code for radix sorting on one bit; changed
 * if (DEBUG) to #if DEBUG to avoid dead code warnings; worked around
 * pointer-loop bug in some icc versions; rerolled some loops for
 * performance; added 'n' argument to scatter() for timing reasons; removed
 * radix parameter 'r' from usortx() and improved radix management;
 * replaced 'uniform' parameter of usortx() with 'flags' and added
 * UNSTABLE and BIGPAGES flags.
 *
 * Revision 0.2  2005/12/30 01:12:32  fmmaley
 * Massive changes to enable in-place sorting & partitioning and sorting
 * under a mask.  Functions usortx and nusortx consolidated into a single
 * entry point usortx.
 *
 * Revision 0.1  2005/12/19 22:38:38  fmmaley
 * Original code written January 2001
 * Updated August 2002 to add multiword support
 * Updated December 2005 to add halfword support
 *
 */
