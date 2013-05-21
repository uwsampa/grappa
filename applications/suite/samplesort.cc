#define THREADS 80
#define MASK ((((unsigned long long int)1) << 53) - 1)
#define HASH_SIZE 8191
#define SIZE 256
#define RAND_BUF_LEN (1024*1024)
#define REPLICATE 32
#include <stdlib.h>
#include <stdio.h>
#define TERA_CLOCK(x) random()
#include <math.h>


#pragma tera parallel off
#pragma tera expect parallel
void serial_quick_sort(unsigned *left, unsigned *right) {
  while (left + 9 < right) {
    unsigned splitter;
    unsigned *l = left;
    unsigned *r = right;
    {
      unsigned ran = TERA_CLOCK(0) & MASK;
      ran %= right - left + 1;
      splitter = left[ran];
    }
    do {
      while (*l < splitter) l++;
      while (*r > splitter) r--;
      if (l <= r) {
        unsigned temp = *l;
        *l++ = *r;
        *r-- = temp;
      }
    } while (l <= r);
    
    if (r - left > right - l) {
      serial_quick_sort(l, right);
      right = r;
    }
    else {
      serial_quick_sort(left, r);
      left = l;
    }
    
  }
  while (left < right) {
    unsigned *minp = left;
    unsigned min = *minp;
    unsigned old_left = min;
    unsigned *p = left + 1;
    while (p <= right) {
      unsigned x = *p;
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



void simple_sort(unsigned *src, unsigned *dst, unsigned n) {
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
  
  #pragma tera assert nodep
  for (unsigned i = 0; i < n; i++)
    dst[count[i]] = src[i];
  
  delete [] count;
}



void sample_sort(unsigned *src, unsigned *dst, unsigned n) {
#pragma noalias *src, *dst
  unsigned sn = sqrt(n);
  unsigned *splitter = new unsigned[sn];
  unsigned *split    = new unsigned[sn];
  unsigned *count    = new unsigned[sn + 1];
  unsigned *start    = new unsigned[sn + 1];
  #pragma tera assert parallel
  for (unsigned i = 0; i < sn; i++)
    splitter[i] = src[(random() & MASK) % n];
  
  for (unsigned i = 0; i < sn; i++)
    count[i] = 0;
  
  for (unsigned i = 1; i < sn; i++)
    for (unsigned j = 0; j < i; j++)
      if (splitter[i] < splitter[j])
        count[j]++;
      else
        count[i]++;
  
  #pragma tera assert nodep
  for (unsigned i = 0; i < sn; i++)
    split[count[i]] = splitter[i];
  
  unsigned bucket;
  unsigned buckets = sn + 1;
  for (unsigned i = 0; i < buckets; i++)
    count[i] = 0;
    
  
  for (unsigned i = 0; i < n; i++) {
    unsigned value = src[i];
    unsigned left = 0;
    unsigned right = sn;
    while (left < right) {
      unsigned midpoint = (left + right)/2;
      unsigned midval = split[midpoint];
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
    
  
  #pragma tera assert nodep dst
  for (unsigned i = 0; i < n; i++) {
    unsigned value = src[i];
    unsigned left = 0;
    unsigned right = sn;
    while (left < right) {
      unsigned midpoint = (left + right)/2;
      unsigned midval = split[midpoint];
      if (value < midval)
	right = midpoint;
      else if (value > midval)
	left = midpoint + 1;
      else
	left = right = midpoint;
    }
    bucket = right;
    value = start[bucket]++;
    dst[value] = src[i];
  }

  #pragma tera assert parallel
  for (unsigned i = 0; i < buckets; i++) {
    unsigned first = start[i] - count[i];
    unsigned last = start[i] - 1;
    serial_quick_sort(dst + first, dst + last);
  }
  
  
  delete [] splitter;
  delete [] split;
  delete [] count;
  delete [] start;
}
main() {
  int NR = 1000;
  unsigned * src = new unsigned[NR];
  unsigned * dst = new unsigned[NR];
  for (int i = 0; i < NR; i++) src[i] = ((i+1)*13134134131)%NR;
  sample_sort(src, dst, NR);
  for (int i = 0; i < NR; i++) fprintf(stderr, "%d ", dst[i]); fprintf(stderr, "\n");
}
