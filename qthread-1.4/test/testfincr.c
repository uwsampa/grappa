#include <stdio.h>
#include <assert.h>
#include <qthread/qthread.h>
#include "argparsing.h"

volatile float master[3] = { 0 };

static aligned_t incr(qthread_t * me, void *arg)
{
    qthread_fincr(master + 1, 1.0);
    return 0;
}

static aligned_t incr5(qthread_t * me, void *arg)
{
    qthread_fincr(master + 1, 5.0);
    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    aligned_t rets[30];
    float ret_test = 4.5;
    qthread_t *me;

    qthread_initialize();
    me = qthread_self();
    iprintf("%i shepherds\n", qthread_num_shepherds());

    assert(master[1] == 0.0);
    ret_test = qthread_fincr(master + 1, 1);
    assert(master[1] == 1.0);
    assert(ret_test == 0.0);
    iprintf("basic increment succeeded\n");
    master[1] = 0;
    for (i = 0; i < 30; i++) {
	qthread_fork(incr, NULL, &(rets[i]));
    }
    for (i = 0; i < 30; i++) {
	qthread_readFF(me, NULL, rets + i);
    }
    if (master[1] != 30.0) {
	fprintf(stderr, "master is %f rather than 30\n", master[1]);
	fprintf(stderr, "master[0]:%f master[1]:%f master[2]:%f\n", master[0],
		master[1], master[2]);
    }
    assert(master[1] == 30.0);
    iprintf("30 concurrent threads successfully incremented by 1\n");
    master[1] = 0.0;
    for (i = 0; i < 30; i++) {
	qthread_fork(incr5, NULL, &(rets[i]));
    }
    for (i = 0; i < 30; i++) {
	qthread_readFF(me, NULL, rets + i);
    }
    if (master[1] != 150.0) {
	fprintf(stderr, "master is %f rather than 150\n", master[1]);
	fprintf(stderr, "master[0]:%f master[1]:%f master[2]:%f\n", master[0],
		master[1], master[2]);
    }
    assert(master[1] == 150.0);
    iprintf("30 concurrent threads successfully incremented by 5\n");

    return 0;
}
