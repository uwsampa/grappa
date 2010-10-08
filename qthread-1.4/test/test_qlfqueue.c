#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <qthread/qthread.h>
#include <qthread/qlfqueue.h>
#include "argparsing.h"

static int elementcount = 10000;
static int threadcount = 128;

static aligned_t queuer(qthread_t * me, void *arg)
{
    qlfqueue_t *q = (qlfqueue_t *) arg;
    size_t i;

    for (i = 0; i < elementcount; i++) {
	if (qlfqueue_enqueue(me, q, (void *)me) != QTHREAD_SUCCESS) {
	    fprintf(stderr, "qlfqueue_enqueue(q, %p) failed!\n", (void *)me);
	    exit(-2);
	}
    }
    return 0;
}

static aligned_t dequeuer(qthread_t * me, void *arg)
{
    qlfqueue_t *q = (qlfqueue_t *) arg;
    size_t i;

    for (i = 0; i < elementcount; i++) {
	while (qlfqueue_dequeue(me, q) == NULL) {
	    qthread_yield(me);
	}
    }
    iprintf("dequeuer %i exiting\n", qthread_id(me));
    return 0;
}

int main(int argc, char *argv[])
{
    qlfqueue_t *q;
    qthread_t *me;
    size_t i;
    aligned_t *rets;

    assert(qthread_initialize() == 0);
    me = qthread_self();
    NUMARG(threadcount, "THREAD_COUNT");
    NUMARG(elementcount, "ELEMENT_COUNT");
    CHECK_VERBOSE();
    iprintf("%i shepherds\n", qthread_num_shepherds());

    if ((q = qlfqueue_create()) == NULL) {
	fprintf(stderr, "qlfqueue_create() failed!\n");
	exit(-1);
    }

    if (qlfqueue_enqueue(me, q, (void *)(intptr_t) 1) != 0) {
	fprintf(stderr, "qlfqueue_enqueue() failed!\n");
	exit(-1);
    }

    if (qlfqueue_dequeue(me, q) != (void *)(intptr_t) 1) {
	fprintf(stderr, "qlfqueue_dequeue() failed!\n");
	exit(-1);
    }

    for (i = 0; i < elementcount; i++) {
	if (qlfqueue_enqueue(me, q, (void *)(intptr_t) (i + 1)) != 0) {
	    fprintf(stderr, "qlfqueue_enqueue(q,%i) failed!\n", (int)i);
	    exit(-1);
	}
    }
    for (i = 0; i < elementcount; i++) {
	if (qlfqueue_dequeue(me, q) != (void *)(intptr_t) (i + 1)) {
	    fprintf(stderr, "qlfqueue_dequeue() failed, didn't equal %i!\n",
		    (int)i);
	    exit(-1);
	}
    }
    if (!qlfqueue_empty(q)) {
	fprintf(stderr, "qlfqueue not empty after ordering test!\n");
	exit(-1);
    }
    iprintf("ordering test succeeded\n");

    rets = (aligned_t *) calloc(threadcount, sizeof(aligned_t));
    assert(rets != NULL);
    for (i = 0; i < threadcount; i++) {
	assert(qthread_fork(dequeuer, q, &(rets[i])) == QTHREAD_SUCCESS);
    }
    iprintf("dequeuers forked\n");
    for (i = 0; i < threadcount; i++) {
	assert(qthread_fork(queuer, q, NULL) == QTHREAD_SUCCESS);
    }
    iprintf("queuers forked\n");
    for (i = 0; i < threadcount; i++) {
	iprintf("waiting for %i\n", i);
	assert(qthread_readFF(me, NULL, &(rets[i])) == QTHREAD_SUCCESS);
    }
    free(rets);
    if (!qlfqueue_empty(q)) {
	fprintf(stderr, "qlfqueue not empty after threaded test!\n");
	exit(-2);
    }
    iprintf("threaded test succeeded\n");

    if (qlfqueue_destroy(me, q) != QTHREAD_SUCCESS) {
	fprintf(stderr, "qlfqueue_destroy() failed!\n");
	exit(-2);
    }

    iprintf("success!\n");

    return 0;
}
