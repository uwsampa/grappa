#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

uint64_t generate(long int n, double *inputs, double accuracy) {
  uint64_t before, after;
  before = get_ns();
#if (NTHR == 0)
  for (long int i = 0; i < n; ++i) {
    int iterations = 0;
    double guess = 0.1;
    double input = (i+1)*10; //inputs[i];
    double err = guess*guess-input;
    while (fabs(err) > accuracy) {
      //      iterations++;
      guess = guess - err/(2*guess);
      err = guess*guess-input;
    }
    //    printf("sqrt(%f) = %f (%d iters)\n", input, guess, iterations);
    inputs[i % 4] = guess;
  }
#else
  long int i = 0;
  #include "newton_unrolled.cunroll"
#endif
  after = get_ns();
  return (after - before);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <num_to_generate> <accuracy>\n", argv[0]);
    exit(1);
  }
  long int n = strtoll(argv[1], NULL, 0);
  double accuracy = strtod(argv[2], NULL);
  /*  double inputs[n];
  for (int i = 0; i < n; ++i) {
    inputs[i] = (i+1)*10;
    }*/
  double inputs[4];
  uint64_t elapsed = generate(n, inputs, accuracy);
  double rate = n*1000;
  rate /= elapsed;
  printf("Generated %lu in %lu ns (%d threads): %f M/s\n",
         n, elapsed, NTHR, rate);
}
