#include <qthread/qthread.h>
#include <qthread/qtimer.h>
#include <time.h>

#include "argparsing.h"

int nyields = 1000*1000;
static aligned_t qincr(qthread_t * me, void *arg)
{
    aligned_t id = (aligned_t) arg;
    size_t incrs, iter;
    // this matter?
    int nyieldslocal = nyields;
    for (iter = 0; iter < nyieldslocal; iter++) {
      qthread_yield(me);
    }
    return 0;
}

int main(int argc, char *argv[])
{
  int nthreads = 10;
  CHECK_VERBOSE();
  NUMARG(nyields, "NYIELDS");
  NUMARG(nthreads, "NTHREADS");
  printf("%d %d\n",nyields, nthreads);
    aligned_t rets[nthreads];
    qthread_t *me;
    qtimer_t timer = qtimer_create();
    double cumulative_time = 0.0;

    if (qthread_initialize() != QTHREAD_SUCCESS) {
	fprintf(stderr, "qthread library could not be initialized!\n");
	exit(EXIT_FAILURE);
    }

    CHECK_VERBOSE();
    me = qthread_self();

    for (int iteration = 0; iteration < 10; iteration++) {
	qtimer_start(timer);
	//for (int i=0; i<NUM_THREADS; i++) {
	for (int i = nthreads - 1; i >= 0; i--) {
	    qthread_fork(qincr, (void *)(intptr_t) (i), &(rets[i]));
	}
	for (int i = 0; i < nthreads; i++) {
	    qthread_readFF(me, NULL, &(rets[i]));
	}
	qtimer_stop(timer);
	iprintf("\ttest iteration %i: %f secs\n", iteration,
		qtimer_secs(timer));
	cumulative_time += qtimer_secs(timer);
    }
    double average_time = cumulative_time / 10.0;
    printf("qthread time: %f (%6f us)\n", average_time, (average_time * 1000 * 1000) / (nthreads * nyields));

    return 0;
}
