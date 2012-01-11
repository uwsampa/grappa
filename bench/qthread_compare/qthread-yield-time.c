
#include <stdio.h>
#include <time.h>
#include <qthread.h>

struct args {
  int count; 
};

static aligned_t yielder( void * av ) {
  struct args * a = (struct args *) av;
  int i;
  for( i = a->count; i > 0; --i ) {
    //printf("In\n");
    qthread_yield();
  }
  return 0;
}
  

int main() {

  qthread_initialize();

  int count = 100000000;

  struct timespec start;
  struct timespec end;
     
  aligned_t ret;
  struct args a = { count };
  qthread_fork(yielder, &a, &ret);

  printf("shepherds=%d, workers=%d\n", qthread_num_shepherds(), qthread_num_workers() );

  clock_gettime(CLOCK_MONOTONIC, &start);

  qthread_readFF(NULL, &ret);

  clock_gettime(CLOCK_MONOTONIC, &end);

  int64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

  double ns_per_yield = (double) runtime_ns / count;

  printf("%d yields in %ld ns = %g ns per yield\n", count, runtime_ns, ns_per_yield );

  qthread_finalize();
  printf("Done.\n");
  return 0;
}
