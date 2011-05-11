#include <assert.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <inttypes.h>
#include "unroll.h"

static uint64_t get_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  //  clock_gettime(CLOCK_REALTIME, &ts);
  uint64_t ns = 0;
  ns += ts.tv_nsec;
  ns += ts.tv_sec * (1000LL * 1000LL * 1000LL);
  return ns;
}

double chase(double a, double b, int niter) {
  double orig_a = a;
  for (; niter > 0; --niter) {
    if (a > 0) {
      a -= b;
      a -= b;
      a -= b;
      a -= b;
      a -= b;

      a -= b;
      a -= b;
      a -= b;
      a -= b;
      a -= b;
    } else {
      a = orig_a;
    }
  }
  return a;
}

double threaded_chase(double orig_a, double b, int niter) {
  assert (niter % NTHR == 0);
  int neach = niter / NTHR;
  double sum = 0;
  #include "mathchase_unrolled.cunroll"
  return sum;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <niter> <divisor>\n", argv[0]);
    exit(1);
  }
  int niter = strtol(argv[1], NULL, 0);
  double start = pow(2,10) + 0.01;
  double divisor = strtod(argv[2], NULL);
  if (niter == 31337) {
    start = 3;
  }
  uint64_t before = get_ns();
  double res;
  if (niter < 0) {
    niter = -niter;
    res = chase(start, divisor, niter);
  } else {
    res = threaded_chase(start, divisor, niter);
  }
  uint64_t after = get_ns();
  uint64_t elapsed = after - before;
  double latency = elapsed;
  latency /= niter;
  latency /= 10;//UNROLL;
  printf("ignore: %f\n", res);
  printf("latency: %f ns %d %d %s\n", latency, UNROLL, NTHR, argv[1]);
}
