/* Simple gups for the CrayXMT -- corresponds to grappa's Gups_tests.cpp
** compile:
**   c++ -o gups gups.cc -par
** run: (increment a 1<<30 word array  2B times)
**   gups 30 2000000000
*/
#include <stdio.h>
#include <stdlib.h>
#include <machine/runtime.h>
#include <sys/mta_task.h>
class timer {
  int t;
  int iters;
  double freq;
public:
  timer(int n) {iters = n; freq = mta_clock_freq();}
#pragma mta no inline
  void start() { t = mta_get_clock(0); }
#pragma mta no inline
  ~timer() {t = mta_get_clock(t); fprintf(stderr, "%d %d %g %g gups\n", iters, t, freq, (iters/1000000000.0)/(t/freq));}
};
#pragma mta no inline
void gups(int *A, unsigned int size, int n) {
  const unsigned int LARGE_PRIME = 18446744073709551557UL;
  unsigned int b;

#pragma mta use 100 streams
  for (int i = 0; i < n; i++) {
    b = (i*LARGE_PRIME) & (size-1);
    A[b]++;
  }
}
main(int argc, char * argv[]) {
  if (argc < 3) fprintf (stderr, "usage: <logsize> <iterations>\n");
  unsigned int size = 1<<(atoi(argv[1]));
  int n = atoi(argv[2]);
  fprintf(stderr, "Running gups on array of size %d for %d iterations\n", size, n);
  int * A = (int *) malloc(sizeof(int)*size);

  for (int i = 0; i < 2; i++) {
    fprintf(stderr, "Timing...\n");
    class timer *t = new timer(n);
    t->start();
    gups(A, size, n);
    delete t;
  }
}

