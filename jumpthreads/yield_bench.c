#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

uint64_t yield_test(long long nyields) {
  uint64_t before = get_ns();
  uint64_t after;
  int from = 0;
  #include "yield_unrolled.cunroll"

  after = get_ns();
  return (after - before);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <nyields (per thread)>\n", argv[0]);
    exit(1);
  }

  long long nyields = strtoll(argv[1], NULL, 0);
  uint64_t elapsed = yield_test(nyields);
  double avg = elapsed;
  avg /= nyields;
  printf("%f ns/yield (%d threads)\n", avg, NTHR);
}
