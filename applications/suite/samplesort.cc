#define THREADS 80
#define MASK ((((unsigned long long int)1) << 53) - 1)
#define HASH_SIZE 8191
#define SIZE 256
#define RAND_BUF_LEN (1024*1024)
#define REPLICATE 32
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifdef __MTA__
#include <machine/mtaops.h>
#include <mta_rng.h>
#else
#define MTA_CLOCK(0) random()
#endif

#pragma mta parallel off
#pragma mta expect parallel
void serial_quick_sort(double *left, double *right, unsigned parity) {
  while (left < right) {
    double splitter;
    double *l = left;
    double *r = right;
    {
      unsigned ran = MTA_CLOCK(0) & MASK;
      ran %= (right - left)/2 + 1;
      splitter = left[2*ran+parity];
    }
    do {
      while (l[parity] < splitter) l+=2;
      while (r[parity] > splitter) r-=2;
      if (l <= r) {
        double temp0 = l[0], temp1 = l[1];
        l[0] = r[0], l[1] = r[1];
        l+=2;
        r[0] = temp0, r[1] = temp1; 
	r-=2;
      }
    } while (l <= r);
    
    if (r - left > right - l) {
      serial_quick_sort(l, right, parity);
      right = r;
    }
    else {
      serial_quick_sort(left, r, parity);
      left = l;
    }
    
  }
  /* not executed: */
  while (left < right) {
    double *minp = left;
    double min = *minp;
    double old_left = min;
    double *p = left + 1;
    while (p <= right) {
      double x = *p;
      if (x < min) {
        min = x;
        minp = p;
      }
      p++;
    }
    *minp = old_left;
    *left++ = min;
  }
  
}

#pragma mta parallel default

void simple_sort(double *src, double *dst, unsigned n) {
#pragma noalias *src, *dst
  unsigned *count = new unsigned[n];
  for (unsigned i = 0; i < n; i++)
    count[i] = 0;
  
  for (unsigned i = 1; i < n; i++)
    for (unsigned j = 0; j < i; j++)
      if (src[i] < src[j])
        count[j]++;
      else
        count[i]++;
  
  #pragma mta assert nodep
  for (unsigned i = 0; i < n; i++)
    dst[count[i]] = src[i];
  
  delete [] count;
}


/* Records are two double words long.  Keys are src[2*i+parity] */
void sample_sort_pairs(double *src, double *dst, unsigned n, unsigned parity) {
#pragma noalias *src, *dst
  unsigned sn = sqrt(n);
  double *splitter = new double[sn];
  double *split    = new double[sn];
  unsigned *count    = new unsigned[sn + 1];
  unsigned *start    = new unsigned[sn + 1];
#ifdef __MTA__
  prand_int(sn, (int *) count);//temp use of count
#endif
  #pragma mta assert parallel
  for (unsigned i = 0; i < sn; i++)
#ifdef __MTA__
    splitter[i] = src[2*((count[i] & MASK) % n)+parity];
#else
    splitter[i] = src[2*(random() % n)+parity];
#endif
  for (unsigned i = 0; i < sn; i++)
    count[i] = 0;
  
  for (unsigned i = 1; i < sn; i++)
    for (unsigned j = 0; j < i; j++)
      if (splitter[i] < splitter[j])
        count[j]++;
      else
        count[i]++;
  
  #pragma mta assert nodep
  for (unsigned i = 0; i < sn; i++)
    split[count[i]] = splitter[i];
  
  unsigned bucket;
  unsigned buckets = sn + 1;
  for (unsigned i = 0; i < buckets; i++)
    count[i] = 0;
    
  
  for (unsigned i = 0; i < n; i++) {
    double value = src[2*i+parity];
    unsigned left = 0;
    unsigned right = sn;
    while (left < right) {
      unsigned midpoint = (left + right)/2;
      double midval = split[midpoint];
      if (value < midval)
	right = midpoint;
      else if (value > midval)
	left = midpoint + 1;
      else
	left = right = midpoint;
    }
    bucket = right;
    count[bucket]++;
  }
  
  start[0] = 0;
  for (unsigned i = 1; i < buckets; i++)
    start[i] = start[i - 1] + count[i - 1];
    
  
  #pragma mta assert parallel
  for (unsigned i = 0; i < n; i++) {
    double value = src[2*i+parity];
    unsigned left = 0;
    unsigned right = sn;
    while (left < right) {
      unsigned midpoint = (left + right)/2;
      double midval = split[midpoint];
      if (value < midval)
	right = midpoint;
      else if (value > midval)
	left = midpoint + 1;
      else
	left = right = midpoint;
    }
    bucket = right;
    int j = int_fetch_add(&start[bucket],1);
    dst[2*j] = src[2*i];
    dst[2*j+1] = src[2*i+1];
  }

  //
  int min = 999999999, max = -1, avg = 0;
  for (int i = 0; i < buckets; i++) {
    avg += count[i];
    min = count[i] < min ? count[i] : min;
    max = count[i] > max ? count[i] : max;
  }
  fprintf(stderr, "bucket size min %d, max %d, avg %d\n", min, max, avg/buckets);
  //

  #pragma mta assert parallel
  for (unsigned i = 0; i < buckets; i++) {
    unsigned first = start[i] - count[i];
    unsigned last = start[i] - 1;
    serial_quick_sort(dst + 2*first, dst + 2*last, parity);
  }
  
  
  delete [] splitter;
  delete [] split;
  delete [] count;
  delete [] start;
}
int test() {
  int NR = 100;
  double * src = new double[2*NR];
  double * dst = new double[2*NR];
  for (int i = 0; i < NR; i++) src[2*i] = ((2*i+1)*13134134131)%12;
  for (int i = 0; i < NR; i++) src[2*i+1] = ((2*i+2)*13134134131)%(2*NR);
  sample_sort_pairs(src, dst, NR, 1);
  for (int i = 0; i < NR; i++) fprintf(stderr, "(%d %d) ", int(dst[2*i]),int(dst[2*i+1])); fprintf(stderr, "\n");
  for (int i = 0; i < NR; i++) dst[2*i] += (1.0*i)/NR;
  for (int i = 0; i < NR; i++) fprintf(stderr, "(%g %g) ", dst[2*i],dst[2*i+1]); fprintf(stderr, "\n");
  sample_sort_pairs(dst, src, NR, 0);
  for (int i = 0; i < NR; i++) fprintf(stderr, "(%d %d) ", int(src[2*i]),int(src[2*i+1])); fprintf(stderr, "\n");
  return 0;
}

