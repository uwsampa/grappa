/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: uhash.c,v 1.2 2008/03/17 18:37:25 fmmaley Exp fmmaley $ */

#include <bitops.h>
#include <stdio.h>
#include "utypes.h"
#include "uhash_x.h"

// Appropriate prefetching for hash table access
#if (defined(__ia64) || defined(__ia64__))
#define PREFETCH _prefetch
#define PREDIST 8
#else
#define PREFETCH(a)
#define PREDIST 0
#endif

// Removal of items with unmatched keys via single-bit hashing, reading
// from a[0:n-1] and writing to c[0:n-1].  (Despite the restriction, c
// and a may be identical.)  The scratch array b[] must hold n>>1 keys.
// Assumes the hash table ht[0:m] has been cleared.  The key consists of
// the bits under the mask ((m<<6)|63) << lb, which should be a single
// interval of bits.

#defcases (sort_t,key_t) = (shw,uint32), (s1w,uint64), (s2w,uint64), (s3w,uint64)
int64
remunm_##sort_t(sort_t* restrict a, int64 n, int64 lb, uint64 m,
		uint64* restrict ht, key_t* restrict b, sort_t* restrict c)
{
  key_t d;
  int64 i, k;

  // printf("remunm_##sort_t: n=%ld, lb=%ld, m=%016lx\n", n, lb, m);
  k = 0;
  for (i = 0; i < n - PREDIST; i++) {
    PREFETCH(ht + (a[i+PREDIST].key>>(lb+6) & m));
    b[k] = d = a[i].key;
    d >>= lb;
    if (ht[d>>6 & m] & UI64(1)<<(d&63))
      k++;
    ht[d>>6 & m] ^= UI64(1)<<(d&63);
  }
#if PREDIST > 0
  for (; i < n; i++) {
    b[k] = d = a[i].key;
    d >>= lb;
    if (ht[d>>6 & m] & UI64(1)<<(d&63))
      k++;
    ht[d>>6 & m] ^= UI64(1)<<(d&63);
  }
#endif
  for (i = 0; i < k; i++) {
    d = b[i] >> lb;
    ht[d>>6 & m] &= ~(UI64(1)<<(d&63));
  }
  k = 0;
  for (i = 0; i < n - PREDIST; i++) {
    PREFETCH(ht + (a[i+PREDIST].key>>(lb+6) & m));
    d = a[i].key >> lb;
    if ( !(ht[d>>6 & m] & UI64(1)<<(d&63)) )
      c[k++] = a[i];
  }
#if PREDIST > 0
  for (; i < n; i++)
    c[k++] = a[i];
#endif
  return k;
}
#endcases
