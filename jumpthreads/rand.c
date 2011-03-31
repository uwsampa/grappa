#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timeb.h>
#include <inttypes.h>

static uint64_t get_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t ns = 0;
  ns += ts.tv_nsec;
  ns += ts.tv_sec * (1000LL * 1000LL * 1000LL);
  return ns;
}

// algorithm matching that used by random_r in glibc
uint64_t generate(long int n) {
  uint64_t before, after;
  before = get_ns();
  int sum = 0;
#if NTHR == 67
  srandom(31337);
  for (int i = 0; i < n; ++i) {
    int result = random();
  }
  sum += random();
#elif NTHR == 0
  //  srandom(3);
  int32_t state[31];
  state[0] = 3;
  int32_t word = 3;
  for (int i = 1; i < 31; ++i) {
    long int hi = word / 127773;
    long int lo = word % 127773;
    word = 16807 * lo - 2836 * hi;
    if (word < 0)
      word += 2147483647;
    state[i] = word;
  }
  int32_t *fptr = &state[3];
  int32_t *rptr = &state[0];
  int32_t *end = &state[31]; // one off the end
  int32_t val, result;
  for (int i = 0; i < 310; ++i) {
    val = *fptr += *rptr;
    result = (val >> 1) & 0x7fffffff;
    ++fptr;
    if (fptr >= end) {
      fptr = state;
      ++rptr;
    } else {
      ++rptr;
      if (rptr >= end) {
        rptr = state;
      }
    }
  }
  for (long int i = 0; i < n; ++i) {
    val = *fptr += *rptr;
    result = (val >> 1) & 0x7fffffff;
    //    assert(result == random());
    ++fptr;
    if (fptr >= end) {
      fptr = state;
      ++rptr;
    } else {
      ++rptr;
      if (rptr >= end) {
        rptr = state;
      }
    }
  }
  sum += state[30];
#else
  long int made = 0;
  #include "rand_unrolled.cunroll"
#endif
  printf("%d\n", sum);
  after = get_ns();
  return (after-before);
}


int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <num_to_generate>\n", argv[0]);
    exit(1);
  }
  long int n = strtoll(argv[1], NULL, 0);
  uint64_t elapsed = generate(n);
  double rate = n*1000;
  rate /= elapsed;
  printf("Generated %lu in %lu ns (%d threads): %f M/s\n",
         n, elapsed, NTHR, rate);
}
