/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: ipsort.c,v 1.1 2008/03/17 18:38:24 fmmaley Exp fmmaley $ */

#include <bitops.h>
#include "utypes.h"
#include "ipsort_x.h"

#defcases IP_SORTTYPE = shw, s1w, s2w, s3w

// Insertion sort for short arrays
void
isort_##IP_SORTTYPE(IP_SORTTYPE *arr, int64 n, uint64 mask)
{
  int64 i, j;
  IP_SORTTYPE ins;

  for (i = 1; i < n; i++) {
    ins = arr[i];
    for (j = i; (j > 0) && ((arr[j-1].key & mask) > (ins.key & mask)); j--)
      arr[j] = arr[j-1];
    arr[j] = ins;
  }
}

// Quicksort-type radix-exchange partitioning for 1-bit radix
int64
qpart_##IP_SORTTYPE(IP_SORTTYPE *arr, int64 n, uint64 bit)
{
  int64 i = 0, j = n-1;
  // We must maintain the invariants 0 <= i < n and 0 <= j < n
  while (i < n-1 && (arr[i].key & bit) == 0) i++;
  while (j > 0 && (arr[j].key & bit) != 0) j--;
  // Now either i=j=0, or i=j=n-1, or i<j and after the first
  // iteration of the main loop we have (arr[0] & bit) == 0
  // and (arr[n-1] & bit) != 0.  This keeps i and j within bounds.
#if (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
  if (i < j) {
    IP_SORTTYPE x, y, *p = arr+i, *q = arr+j;
    uint64 ikey, jkey;
    int64 go;
#pragma swp
    do {
      _prefetch((uint64*)p + 32);
      _prefetch((uint64*)q - 32);
      x = *p; ikey = x.key & bit;
      y = *q; jkey = y.key & bit;
      go = q > (p+1);
      if (ikey != 0 && jkey == 0)
	*(p++) = y, *(q--) = x;
      if (ikey == 0) p++;
      if (jkey != 0) q--;
    } while (go);
    j = q - arr;
  }
#else
  while (i < j) {
    IP_SORTTYPE swap;
    while ((arr[i].key & bit) == 0) i++;
    while ((arr[j].key & bit) != 0) j--;
    if (j <= i) break;
    swap = arr[i];  arr[i] = arr[j];  arr[j] = swap;
    i++, j--;
  }
#endif
  return j + ((arr[j].key & bit) == 0);
}

// One pass of radix-exchange, which partitions the array 'arr' by the
// bits under the mask m<<lb given that m has the form (1<<r)-1 and the
// counts c[0:m] have already been computed.  The counts are not changed.
void
rxpart_##IP_SORTTYPE(IP_SORTTYPE *arr, uint64 *c, int64 lb, uint64 m)
{
  IP_SORTTYPE *a[1<<MAXRX], *b[1<<MAXRX], *p, *q, x, y;
  int64 i, j, k;

  // convert counts to pointers
  p = arr;
  for (j = 0; j <= m; j++)
    a[j] = p, p += c[j];
  for (j = 0; j < m; j++)
    b[j] = a[j+1];
  b[m] = p;

#if (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
  // the double-speed exchange algorithm
  if (c[0] > 0 && c[1] > 0) {
    IP_SORTTYPE x0, x1, y0, y1, *p0, *p1, *q0, *q1;
    int64 k0, k1;
    x0 = *(a[0]++); x1 = *(a[1]++);
    q0 = b[0]; q1 = b[1];
#pragma swp
    do {
      k0 = x0.key>>lb & m;
      k1 = x1.key>>lb & m;
      p0 = a[k0]++;
      p1 = a[k1]++;
      y0 = x0; x0 = *p0;
      y1 = x1; x1 = *p1;
      _prefetch((uint8*)p0 + 64);
      _prefetch((uint8*)p1 + 64);
      *(k0 < 2 ? p0-1 : p0) = y0;
      *(k1 < 2 ? p1-1 : p1) = y1;
    } while (((p0 != q0 && p0 != q1) || k0 >= 2) &&
	     ((p1 != q0 && p1 != q1) || k1 >= 2));
    // clean up here!
    if (a[0] > q0) a[0] = q0;
    else *(--a[0]) = (k1 == 1 ? x0 : x1);
    if (a[1] > q1) a[1] = q1;
    else *(--a[1]) = (k1 == 0 ? x0 : x1);
  }
#endif

  // the straightforward exchange algorithm
  for (i = 0; i <= m; i++) {
    while (a[i] < b[i]) {
      q = a[i];
      _prefetch((uint8*)q + 64);
      x = *q;
      while ((k = x.key>>lb & m) != i) {
	p = a[k]++;
	_prefetch((uint8*)p + 64);
	y = *p, *p = x, x = y;
      }
      *(a[i]++) = x;
    }
  }
}

#endcases


#defcases IP_SORTTYPE = shw, s2w, s3w

// Insertion sort from s[] to t[] for arrays that are nearly sorted
void
nisort_##IP_SORTTYPE(IP_SORTTYPE *s, IP_SORTTYPE *t, int64 n, uint64 m)
{
  int64 i, j;
#if (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
  IP_SORTTYPE a, w, x, y, z = { 0 };

  z = (n > 0) ? s[0] : z;
  y = (n > 1) ? s[1] : z;
  if ((z.key & m) > (y.key & m)) w = y, y = z, z = w;
  if (n < 3) {
    if (n > 0) t[0] = z;
    if (n > 1) t[1] = y;
    return;
  }
  x = s[2];
  if ((y.key & m) > (x.key & m)) w = x, x = y, y = w;
  if ((z.key & m) > (y.key & m)) w = y, y = z, z = w;

  for (i = 3; i < n; i++) {
    //#pragma noswp
    for (; i < n; i++) {
      w = s[i];
      t[i-3] = z;
      if ( !((w.key & m) >= (z.key & m)) ) break;
      z = (w.key & m) >= (y.key & m) ? y : w;
      a = (w.key & m) >= (x.key & m) ? x : w;
      y = (w.key & m) >= (y.key & m) ? a : y;
      x = (w.key & m) >= (x.key & m) ? w : x;
    }
    if (i < n) {
#pragma loop count (1)
      for (j = i-3; j > 0 && (t[j-1].key & m) > (w.key & m); j--)
	t[j] = t[j-1];
      t[j] = w;
    }
  }

  t[n-3] = z, t[n-2] = y, t[n-1] = x;
#else
  IP_SORTTYPE w;
  for (i = 0; i < n; i++) {
    w = s[i];
    for (j = i; j > 0 && (t[j-1].key & m) > (w.key & m); j--)
      t[j] = t[j-1];
    t[j] = w;
  }
#endif
}
#endcases


void
nisort_uint64(uint64 *s, uint64 *t, int64 n, uint64 m)
{
  int64 i, j;
#if (defined(__ia64) || defined(__ia64__)) && defined(__INTEL_COMPILER)
  uint64 a, w, x, y, z = { 0 };

  z = (n > 0) ? s[0] : z;
  y = (n > 1) ? s[1] : z;
  if ((z & m) > (y & m)) w = y, y = z, z = w;
  if (n < 3) {
    if (n > 0) t[0] = z;
    if (n > 1) t[1] = y;
    return;
  }
  x = s[2];
  if ((y & m) > (x & m)) w = x, x = y, y = w;
  if ((z & m) > (y & m)) w = y, y = z, z = w;

  for (i = 3; i < n; i++) {
#pragma noswp
    for (; i < n; i++) {
      w = s[i];
      t[i-3] = z;
      if ( !((w & m) >= (z & m)) ) break;
      z = (w & m) >= (y & m) ? y : w;
      a = (w & m) >= (x & m) ? x : w;
      y = (w & m) >= (y & m) ? a : y;
      x = (w & m) >= (x & m) ? w : x;
    }
    if (i < n) {
#pragma loop count (1)
      for (j = i-3; j > 0 && (t[j-1] & m) > (w & m); j--)
	t[j] = t[j-1];
      t[j] = w;
    }
  }

  t[n-3] = z, t[n-2] = y, t[n-1] = x;
#else
  uint64 w;
  for (i = 0; i < n; i++) {
    w = s[i];
    for (j = i; j > 0 && (t[j-1] & m) > (w & m); j--)
      t[j] = t[j-1];
    t[j] = w;
  }
#endif
}
