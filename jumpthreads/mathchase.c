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
    if (a > 1) {
      a /= b;
    } else {
      a = orig_a;
    }
  }
  return a;
}


int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <niter> <divisor>\n", argv[0]);
    exit(1);
  }
  int niter = strtol(argv[1], NULL, 0);
  double start = pow(2,100);
  double divisor = strtod(argv[2], NULL);
  if (niter == 31337) {
    start = 3;
  }
  uint64_t before = get_ns();
  double res = chase(start, divisor, niter);
  uint64_t after = get_ns();
  uint64_t elapsed = after - before;
  double latency = elapsed;
  latency /= niter;
  printf("ignore: %f\n", res);
  printf("latency: %f ns\n", latency);
}
