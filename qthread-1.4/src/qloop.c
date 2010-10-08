#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdlib.h>
#include <qthread/qthread.h>
#include <qthread/qloop.h>
#include <qt_barrier.h>
#include <qthread_asserts.h>
#include <qthread_innards.h>	       /* for qthread_debug() */
#include <qthread/qtimer.h>

#include "qloop_innards.h"

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
#include "qt_atomics.h"
#endif

/* avoid compiler bugs with volatile... */
static Q_NOINLINE aligned_t vol_read_a(
    volatile aligned_t * ptr)
{
    return *ptr;
}

#define _(x) vol_read_a(&(x))

/* So, the idea here is that this is a (braindead) C version of Megan's
 * mt_loop. */
struct qloop_wrapper_args {
    qt_loop_f func;
    size_t startat, stopat;
    void *arg;
};

static aligned_t qloop_wrapper(
    qthread_t * const restrict me,
    struct qloop_wrapper_args * const restrict arg)
{				       /*{{{ */
    arg->func(me, arg->startat, arg->stopat, arg->arg);
    return 0;
}				       /*}}} */

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
struct qloop_step_wrapper_args {
    qt_loop_step_f func;
    size_t startat, stopat, step;
    void *arg;
};

static aligned_t qloop_step_wrapper(
    qthread_t * const restrict me,
    struct qloop_step_wrapper_args * const restrict arg)
{				       /*{{{ */
    arg->func(me, arg->startat, arg->stopat, arg->step, arg->arg);
    return 0;
}				       /*}}} */
#endif

static void qt_loop_inner(
    const size_t start,
    const size_t stop,
    const qt_loop_f func,
    void *argptr,
    int future)
{				       /*{{{ */
    size_t i, threadct = 0;
    qthread_t *const me = qthread_self();
    size_t steps = stop - start;
    struct qloop_wrapper_args *qwa;
    syncvar_t *rets;

    qwa = (struct qloop_wrapper_args *)
	malloc(sizeof(struct qloop_wrapper_args) * steps);
    rets = calloc(steps, sizeof(syncvar_t));
    assert(qwa);
    assert(rets);
    assert(func);

    for (i = start; i < stop; ++i) {
	qwa[threadct].func = func;
	qwa[threadct].startat = i;
	qwa[threadct].stopat = i + 1;
	qwa[threadct].arg = argptr;
	if (future) {
	    qassert(qthread_fork_syncvar_future_to
		    (me, (qthread_f) qloop_wrapper, qwa + threadct, rets + i,
		     (qthread_shepherd_id_t) (threadct %
			 qthread_num_shepherds())),
		    QTHREAD_SUCCESS);
	} else {
	    qassert(qthread_fork_syncvar_to
		    ((qthread_f) qloop_wrapper, qwa + threadct, rets + i,
		     (qthread_shepherd_id_t) (threadct %
			 qthread_num_shepherds())),
		    QTHREAD_SUCCESS);
	}
	threadct++;
    }
    for (i = 0; i < steps; i++) {
	qthread_syncvar_readFF(me, NULL, rets + i);
    }
    free(qwa);
}				       /*}}} */

void qt_loop(
    const size_t start,
    const size_t stop,
    const qt_loop_f func,
    void *argptr)
{				       /*{{{ */
    qt_loop_inner(start, stop, func, argptr, 0);
}				       /*}}} */

/* So, the idea here is that this is a (braindead) C version of Megan's
 * mt_loop_future. */
void qt_loop_future(
    const size_t start,
    const size_t stop,
    const qt_loop_f func,
    void *argptr)
{				       /*{{{ */
    qt_loop_inner(start, stop, func, argptr, 1);
}				       /*}}} */

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
static void qt_loop_step_inner(
    const size_t start,
    const size_t stop,
    const size_t stride,
    const qt_loop_step_f func,
    void *argptr,
    int future)
{				       /*{{{ */
    size_t i, threadct = 0;
    qthread_t *const me = qthread_self();
    size_t steps = (stop - start) / stride;
    struct qloop_step_wrapper_args *qwa;
    syncvar_t *rets;

    if ((steps * stride) + start < stop) {
	steps++;
    }
    qwa = (struct qloop_step_wrapper_args *)
	malloc(sizeof(struct qloop_step_wrapper_args) * steps);
    rets = calloc(steps, sizeof(syncvar_t));
    assert(qwa);
    assert(rets);
    assert(func);

    for (i = start; i < stop; i += stride) {
	qwa[threadct].func = func;
	qwa[threadct].startat = i;
	qwa[threadct].stopat = i + 1;
	qwa[threadct].step = stride;
	qwa[threadct].arg = argptr;
	if (future) {
	    future_fork_syncvar_to
		((qthread_f) qloop_step_wrapper, qwa + threadct, rets + i,
		 (qthread_shepherd_id_t) (threadct %
					  qthread_num_shepherds()));
	} else {
	    qassert(qthread_fork_syncvar_to
		    ((qthread_f) qloop_step_wrapper, qwa + threadct, rets + i,
		     (qthread_shepherd_id_t) (threadct %
			 qthread_num_shepherds())),
		    QTHREAD_SUCCESS);
	}
	threadct++;
    }
    for (i = 0; i < steps; i++) {
	qthread_syncvar_readFF(me, NULL, rets + i);
    }
    free(qwa);
}				       /*}}} */
#endif /* QTHREAD_USE_ROSE_EXTENSIONS */

/* So, the idea here is that this is a C version of Megan's mt_loop (note: not
 * using futures here (at least, not exactly)). As such, what it needs to do is
 * decide the following:
 * 1. How many threads will we need?
 * 2. How will the work be divided among the threads?
 *
 * The easy option is this: numthreads is the number of shepherds, and we
 * divide work evenly, so that we're maxing out our processor use with minimal
 * overhead. Then, we divvy the work up as evenly as we can.
 */
static QINLINE void qt_loop_balance_inner(
    const size_t start,
    const size_t stop,
    const qt_loop_f func,
    void *argptr,
    const int future)
{				       /*{{{ */
    qthread_shepherd_id_t i;
    const qthread_shepherd_id_t maxsheps = qthread_num_shepherds();
    struct qloop_wrapper_args *const qwa =
	(struct qloop_wrapper_args *)malloc(sizeof(struct qloop_wrapper_args)
					    * maxsheps);
    syncvar_t *const rets = calloc(maxsheps, sizeof(syncvar_t));
    const size_t each = (stop - start) / maxsheps;
    size_t extra = (stop - start) - (each * maxsheps);
    size_t iterend = start;
    qthread_t *const me = qthread_self();

    assert(func);
    assert(qwa);
    for (i = 0; i < maxsheps; i++) {
	qwa[i].func = func;
	qwa[i].arg = argptr;
	qwa[i].startat = iterend;
	qwa[i].stopat = iterend + each;
	if (extra > 0) {
	    qwa[i].stopat++;
	    extra--;
	}
	iterend = qwa[i].stopat;
	if (!future) {
	    qassert(qthread_fork_syncvar_to
		    ((qthread_f) qloop_wrapper, qwa + i, rets + i, i),
		    QTHREAD_SUCCESS);
	} else {
	    qassert(qthread_fork_syncvar_future_to
		    (me, (qthread_f) qloop_wrapper, qwa + i, rets + i, i),
		    QTHREAD_SUCCESS);
	}
    }
    for (i = 0; i < maxsheps; i++) {
	qthread_syncvar_readFF(me, NULL, rets + i);
    }
    free(qwa);
}				       /*}}} */

void qt_loop_balance(
    const size_t start,
    const size_t stop,
    const qt_loop_f func,
    void *argptr)
{				       /*{{{ */
    qt_loop_balance_inner(start, stop, func, argptr, 0);
}				       /*}}} */

void qt_loop_balance_future(
    const size_t start,
    const size_t stop,
    const qt_loop_f func,
    void *argptr)
{				       /*{{{ */
    qt_loop_balance_inner(start, stop, func, argptr, 1);
}				       /*}}} */

struct qloopaccum_wrapper_args {
    qt_loopr_f func;
    size_t startat, stopat;
    void *restrict arg;
    void *restrict ret;
};

static aligned_t qloopaccum_wrapper(
    qthread_t * me,
    const struct qloopaccum_wrapper_args *arg)
{				       /*{{{ */
    arg->func(me, arg->startat, arg->stopat, arg->arg, arg->ret);
    return 0;
}				       /*}}} */

static QINLINE void qt_loopaccum_balance_inner(
    const size_t start,
    const size_t stop,
    const size_t size,
    void *restrict out,
    const qt_loopr_f func,
    void *restrict argptr,
    const qt_accum_f acc,
    const int future)
{				       /*{{{ */
    qthread_shepherd_id_t i;
    struct qloopaccum_wrapper_args *const qwa =
	(struct qloopaccum_wrapper_args *)
	malloc(sizeof(struct qloopaccum_wrapper_args) *
	       qthread_num_shepherds());
    syncvar_t *const rets =
	(syncvar_t *) calloc(qthread_num_shepherds(), sizeof(syncvar_t));
    char *realrets = NULL;
    const size_t each = (stop - start) / qthread_num_shepherds();
    size_t extra = (stop - start) - (each * qthread_num_shepherds());
    size_t iterend = start;
    qthread_t *const me = qthread_self();

    if (qthread_num_shepherds() > 1) {
	realrets = (char *)malloc(size * (qthread_num_shepherds() - 1));
	assert(realrets);
    }
    assert(rets);
    assert(qwa);
    assert(func);
    assert(acc);

    for (i = 0; i < qthread_num_shepherds(); i++) {
	qwa[i].func = func;
	qwa[i].arg = argptr;
	if (i == 0) {
	    qwa[0].ret = out;
	} else {
	    qwa[i].ret = realrets + ((i - 1) * size);
	}
	qwa[i].startat = iterend;
	qwa[i].stopat = iterend + each;
	if (extra > 0) {
	    qwa[i].stopat++;
	    extra--;
	}
	iterend = qwa[i].stopat;
	if (!future) {
	    qassert(qthread_fork_syncvar_to
		    ((qthread_f) qloopaccum_wrapper, qwa + i, rets + i, i),
		    QTHREAD_SUCCESS);
	} else {
	    qassert(qthread_fork_syncvar_future_to
		    (me, (qthread_f) qloopaccum_wrapper, qwa + i, rets + i,
		     i), QTHREAD_SUCCESS);
	}
    }
    for (i = 0; i < qthread_num_shepherds(); i++) {
	qthread_syncvar_readFF(me, NULL, rets + i);
	if (i > 0) {
	    acc(out, realrets + ((i - 1) * size));
	}
    }
    free(rets);
    if (realrets) {
	free(realrets);
    }
    free(qwa);
}				       /*}}} */

void qt_loopaccum_balance(
    const size_t start,
    const size_t stop,
    const size_t size,
    void *restrict out,
    const qt_loopr_f func,
    void *restrict argptr,
    const qt_accum_f acc)
{				       /*{{{ */
    qt_loopaccum_balance_inner(start, stop, size, out, func, argptr, acc, 0);
}				       /*}}} */

void qt_loopaccum_balance_future(
    const size_t start,
    const size_t stop,
    const size_t size,
    void *restrict out,
    const qt_loopr_f func,
    void *restrict argptr,
    const qt_accum_f acc)
{				       /*{{{ */
    qt_loopaccum_balance_inner(start, stop, size, out, func, argptr, acc, 1);
}				       /*}}} */

/* Now, the easy option for qt_loop_balance() is... effective, but has a major
 * drawback: if some iterations take longer than others, we will have a laggard
 * thread holding everyone up. Even worse, imagine if a shepherd is disabled
 * during loop processing: with qt_loop_balance, the thread responsible for a
 * 1/n chunk of the iteration space will be reassigned to another shepherd,
 * thereby guaranteeing that one thread doesn't keep up with the rest (and that
 * we will have idle shepherds).
 *
 * To handle this, we can use a slightly more complicated (and thus,
 * less-efficient) method: a shared iteration "queue" (probably the wrong word,
 * but gives you the right idea) that each thread can pull from. This allows
 * for a certain degree of self-scheduling, and adapts better when shepherds
 * are disabled.
 */

static Q_UNUSED QINLINE int qqloop_get_iterations_guided(
    struct qqloop_iteration_queue *const restrict iq,
    struct qqloop_static_args *const restrict sa,
    struct qqloop_wrapper_range *const restrict range)
{				       /*{{{ */
    saligned_t ret = iq->start;
    saligned_t ret2 = iq->stop;
    saligned_t iterations = 0;
    const saligned_t stop = iq->stop;
    const qthread_shepherd_id_t sheps = sa->activesheps;

    if (qthread_num_shepherds() == 1) {
	if (ret < iq->stop) {
	    range->startat = iq->start;
	    range->stopat = iq->stop;
	    iq->start = iq->stop;
	    return 1;
	} else {
	    range->startat = range->stopat = 0;
	    return 0;
	}
    }

    /* this loop ensure atomicity in figuring out the number of iterations to
     * process */
    while (ret < iq->stop && ret != ret2) {
	ret = iq->start;
	iterations = (stop - ret) / sheps;
	if (iterations == 0) {
	    iterations = 1;
	}
	ret2 = qthread_cas(&(iq->start), ret, ret + iterations);
    }
    if (ret < iq->stop) {
	assert(iterations > 0);
	range->startat = ret;
	range->stopat = ret + iterations;
	return 1;
    } else {
	range->startat = 0;
	range->stopat = 0;
	return 0;
    }
}				       /*}}} */

static QINLINE int qqloop_get_iterations_factored(
    struct qqloop_iteration_queue *const restrict iq,
    struct qqloop_static_args *const restrict sa,
    struct qqloop_wrapper_range *const restrict range)
{				       /*{{{ */
    saligned_t ret = iq->start;
    saligned_t ret2 = iq->stop;
    const saligned_t stop = iq->stop;
    saligned_t iterations = 0;
    saligned_t phase = iq->type_specific_data.phase;
    const qthread_shepherd_id_t sheps = sa->activesheps;

    if (qthread_num_shepherds() == 1) {
	if (ret < iq->stop) {
	    range->startat = iq->start;
	    range->stopat = iq->stop;
	    iq->start = iq->stop;
	    return 1;
	} else {
	    range->startat = range->stopat = 0;
	    return 0;
	}
    }

    /* this loop ensures atomicity in figuring out the number of iterations to
     * process */
    while (ret < iq->stop && ret != ret2) {
	ret = iq->start;
	while (ret >= phase && ret < iq->stop) {
	    /* set a new phase */
	    saligned_t newphase = (stop + ret) / 2;
	    saligned_t chunksize = (stop - newphase) / sheps;
	    newphase = ret + (chunksize * sheps);
	    if (newphase != phase) {
		phase = qthread_cas(&(iq->type_specific_data.phase), phase, newphase);
	    } else {
		phase = qthread_cas(&(iq->type_specific_data.phase), phase, stop);
	    }
	}
	iterations = (stop - phase) / sheps;
	if (iterations == 0) {
	    iterations = 1;
	}
	ret2 = qthread_cas(&(iq->start), ret, ret + iterations);
    }
    if (ret < iq->stop) {
	assert(iterations > 0);
	range->startat = ret;
	range->stopat = ret + iterations;
	return 1;
    } else {
	range->startat = 0;
	range->stopat = 0;
	return 0;
    }
}				       /*}}} */

static QINLINE int qqloop_get_iterations_chunked(
    struct qqloop_iteration_queue *const restrict iq,
    struct qqloop_static_args *const restrict sa,
    struct qqloop_wrapper_range *const restrict range)
{
    abort();
    return 0;
}

static QINLINE int qqloop_get_iterations_timed(
    struct qqloop_iteration_queue *const restrict iq,
    struct qqloop_static_args *const restrict sa,
    struct qqloop_wrapper_range *const restrict range)
{
    const qthread_shepherd_id_t workerCount = sa->activesheps;
    const qthread_shepherd_id_t shep = qthread_shep(NULL);
    const saligned_t localstop = iq->stop;

    ssize_t dynamicBlock;
    double loop_time;
    saligned_t localstart;

    assert(iq->type_specific_data.timed.timers != NULL);
    assert(iq->type_specific_data.timed.lastblocks != NULL);
    if (range->startat == 0 && range->stopat == 0) {
	loop_time = 1.0;
	dynamicBlock = 1;
    } else {
	loop_time = qtimer_secs(iq->type_specific_data.timed.timers[shep]);
	dynamicBlock = iq->type_specific_data.timed.lastblocks[shep];
    }

    /* this loop ensures atomicity while figuring out iterations */
    localstart = iq->start;
    while (localstart < localstop) {
	saligned_t tmp;
	if (loop_time >= 7.5e-7) { /* KBW: XXX: arbitrary constant */
	    dynamicBlock = ((localstop - localstart)/(workerCount << 1))+1;
	}
	if ((localstart + dynamicBlock) > localstop) {
	    dynamicBlock = (localstop - localstart);
	}
	tmp = qthread_cas(&iq->start, localstart, localstart + dynamicBlock);
	if (tmp == localstart) {
	    iq->type_specific_data.timed.lastblocks[shep] = dynamicBlock;
	    break;
	} else {
	    localstart = tmp;
	}
    }

    if (localstart < localstop && dynamicBlock > 0) {
	assert((localstart + dynamicBlock) <= localstop);
	range->startat = localstart;
	range->stopat = localstart + dynamicBlock;
	return 1;
    } else {
	range->startat = 0;
	range->stopat = 0;
	return 0;
    }
}

static QINLINE struct qqloop_iteration_queue *qqloop_create_iq(
    const size_t startat,
    const size_t stopat,
    const size_t step,
    const qt_loop_queue_type type)
{				       /*{{{ */
    struct qqloop_iteration_queue *iq =
	malloc(sizeof(struct qqloop_iteration_queue));
    iq->start = startat;
    iq->stop = stopat;
    iq->step = step;
    iq->type = type;
    switch (type) {
	case FACTORED:
	    iq->type_specific_data.phase = (startat + stopat) / 2;
	    break;
	case TIMED:
	    {
		const qthread_shepherd_id_t max = qthread_num_shepherds();
		qthread_shepherd_id_t i;
		qtimer_t *timers = malloc(sizeof(qtimer_t) * max);
		assert(timers);
		for (i=0; i<max; i++) {
		    timers[i] = qtimer_create();
		}
		iq->type_specific_data.timed.timers = timers;
		iq->type_specific_data.timed.lastblocks = malloc(sizeof(saligned_t) * max);
		assert(iq->type_specific_data.timed.lastblocks);
	    }
	    break;
	default:
	    break;
    };
    return iq;
}				       /*}}} */

static QINLINE void qqloop_destroy_iq(
    struct qqloop_iteration_queue *iq)
{				       /*{{{ */
    assert(iq);
    switch (iq->type) {
	case TIMED:
	    if (iq->type_specific_data.timed.timers) {
		const qthread_shepherd_id_t max = qthread_num_shepherds();
		qthread_shepherd_id_t i;
		for (i = 0; i < max; i++) {
		    qtimer_destroy(iq->type_specific_data.timed.timers[i]);
		}
		free(iq->type_specific_data.timed.timers);
	    }
	    if (iq->type_specific_data.timed.lastblocks) {
		free(iq->type_specific_data.timed.lastblocks);
	    }
	    break;
	default:
	    break;
    }
    free(iq);
}				       /*}}} */

static aligned_t qqloop_wrapper(
    qthread_t * me,
    const struct qqloop_wrapper_args *arg)
{
    struct qqloop_static_args *const restrict stat = arg->stat;
    struct qqloop_iteration_queue *const restrict iq = stat->iq;
    const qt_loop_f func = stat->func;
    void *const restrict a = stat->arg;
    volatile aligned_t *const dc = &(stat->donecount);
    const qq_getiter_f get_iters = stat->get;
    const qthread_shepherd_id_t shep = arg->shep;

    /* non-consts */
    struct qqloop_wrapper_range range = {0,0};
    int safeexit = 1;

    assert(get_iters != NULL);
    if (qthread_shep(me) == shep && get_iters(iq, stat, &range)) {
	assert(range.startat != range.stopat);
	do {
	    if (iq->type == TIMED) {
		qtimer_start(iq->type_specific_data.timed.timers[shep]);
	    }
	    func(me, range.startat, range.stopat, a);
	    if (iq->type == TIMED) {
		qtimer_stop(iq->type_specific_data.timed.timers[shep]);
	    }
	    if (!qthread_shep_ok(me) || qthread_shep(me) != shep) {
		/* my shepherd has been disabled while I was running */
		qthread_debug(ALL_DETAILS,
			      "my shepherd (%i) has been disabled!\n",
			      (int)shep);
		safeexit = 0;
		qthread_incr(&(stat->activesheps), -1);
		break;
	    }
	} while (get_iters(iq, stat, &range));
    }
    if (safeexit) {
	qthread_incr(dc, 1);
    }
    return 0;
}

qqloop_handle_t *qt_loop_queue_create(
    const qt_loop_queue_type type,
    const size_t start,
    const size_t stop,
    const size_t incr,
    const qt_loop_f func,
    void *const restrict argptr)
{
    qassert_ret(func, NULL);
    {
	qqloop_handle_t *const restrict h = malloc(sizeof(qqloop_handle_t));

	if (h) {
	    const qthread_shepherd_id_t maxsheps = qthread_num_shepherds();
	    qthread_shepherd_id_t i;

	    h->qwa = malloc(sizeof(struct qqloop_wrapper_args) * maxsheps);
	    h->stat.donecount = 0;
	    h->stat.activesheps = maxsheps;
	    h->stat.iq = qqloop_create_iq(start, stop, incr, type);
	    h->stat.func = func;
	    h->stat.arg = argptr;
	    switch(type) {
		case FACTORED:
		    h->stat.get = qqloop_get_iterations_factored; break;
		case TIMED:
		    h->stat.get = qqloop_get_iterations_timed; break;
		case GUIDED:
		    h->stat.get = qqloop_get_iterations_guided; break;
		case CHUNK:
		    h->stat.get = qqloop_get_iterations_chunked; break;
	    }
	    for (i = 0; i < maxsheps; i++) {
		h->qwa[i].stat = &(h->stat);
		h->qwa[i].shep = i;    // this is the only thread-specific piece of information...
	    }
	}
	return h;
    }
}

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
qtrose_loop_handle_t *qtrose_loop_queue_create(
    const qt_loop_queue_type type,
    const size_t start,
    const size_t stop,
    const size_t incr,
    const qt_loop_step_f func,
    void *const restrict argptr)
{
    qassert_ret(func, NULL);
    {
	qtrose_loop_handle_t *const restrict h = malloc(sizeof(qtrose_loop_handle_t));

	if (h) {
	    h->func = func;
	    h->workers = 0;
	    h->shepherdsActive = 0;
	    h->assignNext = start;
	    h->assignStop = stop;
	    h->assignStep = incr;
	    h->assignDone = stop;
	}
	return h;
    }
}
#endif

void qt_loop_queue_run(
    qqloop_handle_t * loop)
{
    qassert_retvoid(loop);
    {
	qthread_shepherd_id_t i;
	const qthread_shepherd_id_t maxsheps = qthread_num_shepherds();
	qthread_t *const me = qthread_self();
	volatile aligned_t *const dc = &(loop->stat.donecount);
	volatile aligned_t *const as = &(loop->stat.activesheps);

	for (i = 0; i < maxsheps; i++) {
	    qthread_fork_to((qthread_f) qqloop_wrapper, loop->qwa + i, NULL,
			    i);
	}
	/* turning this into a spinlock :P
	 * I *would* do readFF, except shepherds can join and leave
	 * during the loop */
	while (_(*dc) < _(*as)) {
	    qthread_yield(me);
	}
	qqloop_destroy_iq(loop->stat.iq);
	free(loop->qwa);
	free(loop);
    }
}

void qt_loop_queue_run_there(
    qqloop_handle_t * loop,
    qthread_shepherd_id_t shep)
{
    qassert_retvoid(loop);
    qassert_retvoid(shep < qthread_num_shepherds());
    {
	qthread_t *const me = qthread_self();
	volatile aligned_t *const dc = &(loop->stat.donecount);
	volatile aligned_t *const as = &(loop->stat.activesheps);

	qthread_fork_to((qthread_f) qqloop_wrapper, loop->qwa + shep, NULL,
			shep);
	/* turning this into a spinlock :P
	 * I *would* do readFF, except shepherds can join and leave
	 * during the loop */
	while (_(*dc) < _(*as)) {
	    qthread_yield(me);
	}
	qqloop_destroy_iq(loop->stat.iq);
	free(loop->qwa);
	free(loop);
    }
}

/* The easiest way to get shepherds/workers to REJOIN when/if shepherds are
 * re-enabled is to make the user do it. */
void qt_loop_queue_addworker(
    qqloop_handle_t * loop,
    const qthread_shepherd_id_t shep)
{
    qthread_incr(&(loop->stat.activesheps), 1);
    if (_(loop->stat.donecount) == 0) {
	qthread_fork_to((qthread_f) qqloop_wrapper, loop->qwa + shep, NULL,
			shep);
    } else {
	qthread_incr(&(loop->stat.activesheps), -1);
    }
}

#define PARALLEL_FUNC(category, initials, _op_, type, shorttype) \
static void qt##initials##_febworker(qthread_t * me, const size_t startat, \
			 const size_t stopat, void * restrict arg, \
			 void * restrict ret) \
{ \
    size_t i; \
    type acc; \
    qthread_readFF(me, NULL, (aligned_t*)(((type *)arg)+startat)); \
    acc = ((type *)arg)[startat]; \
    for (i = startat + 1; i < stopat; i++) { \
	qthread_readFF(me, NULL, (aligned_t*)(((type *)arg) + i)); \
	acc = _op_(acc, ((type *)arg)[i]); \
    } \
    *(type *)ret = acc; \
} \
static void qt##initials##_worker(qthread_t * me, const size_t startat, \
			 const size_t stopat, void * restrict arg, \
			 void * restrict ret) \
{ \
    size_t i; \
    type acc = ((type*)arg)[startat]; \
    for (i = startat + 1; i < stopat; i++) { \
	acc = _op_(acc, ((type *)arg)[i]); \
    } \
    *(type *)ret = acc; \
} \
static void qt##initials##_acc (void * restrict a, void * restrict b) \
{ \
    *(type *)a = _op_(*(type *)a, *(type *)b); \
} \
type qt_##shorttype##_##category (type *array, size_t length, int checkfeb) \
{ \
    type ret; \
    if (checkfeb) { \
    if (sizeof(type) != sizeof(aligned_t)) return 0; \
    qt_loopaccum_balance_inner(0, length, sizeof(type), &ret, \
			 qt##initials##_febworker, \
			 array, qt##initials##_acc, 0 ); \
    } else { \
    qt_loopaccum_balance_inner(0, length, sizeof(type), &ret, \
			 qt##initials##_worker, \
			 array, qt##initials##_acc, 0 ); \
    } \
    return ret; \
}

#define ADD(a,b) a+b
#define MULT(a,b) a*b
#define MAX(a,b) (a>b)?a:b
#define MIN(a,b) (a<b)?a:b

PARALLEL_FUNC(sum, uis, ADD, aligned_t, uint)
PARALLEL_FUNC(prod, uip, MULT, aligned_t, uint)
PARALLEL_FUNC(max, uimax, MAX, aligned_t, uint)
PARALLEL_FUNC(min, uimin, MIN, aligned_t, uint)

PARALLEL_FUNC( sum, is,   ADD, saligned_t, int)
PARALLEL_FUNC(prod, ip,  MULT, saligned_t, int)
PARALLEL_FUNC( max, imax, MAX, saligned_t, int)
PARALLEL_FUNC( min, imin, MIN, saligned_t, int)
PARALLEL_FUNC( sum, ds,   ADD, double, double)
PARALLEL_FUNC(prod, dp,  MULT, double, double)
PARALLEL_FUNC( max, dmax, MAX, double, double)
PARALLEL_FUNC( min, dmin, MIN, double, double)

/* The next idea is to implement it in a memory-bound kind of way. And I don't
 * mean memory-bound in that it spends its time waiting for memory; I mean in
 * the kind of "that memory belongs to shepherd Y, so therefore iteration X
 * should be on shepherd Y".
 *
 * Of course, in terms of giving each processor a contiguous chunk to work on,
 * that's what qt_loop_balance() does. The really interesting bit is to
 * "register" address ranges with the library, and then have it decide where to
 * spawn threads (& futures) based on the array you've handed it. HOWEVER, this
 * can't be quite so generic, unfortunately, because we don't know what memory
 * we're working with (the qt_loop_balance interface is too generic).
 *
 * The more I think about that, though, the more I'm convinced that it's almost
 * impossible to make particularly generic, because an *arbitrary* function may
 * use two or more arrays that are on different processors, and there's no way
 * qthreads can know that (or even do very much to help if that memory has been
 * assigned to different processors). The best way to achieve this sort of
 * behavior is through premade utility functions, like qutil... but even then,
 * binding given memory to given shepherds won't last through multiple calls
 * unless it's done explicitly. That said, given the way this works, doing
 * repeated operations on the same array will divide the array in the same
 * fashion every time.
 */
#define SWAP(a, m, n) do { register double temp=a[m]; a[m]=a[n]; a[n]=temp; } while (0)
    static int dcmp(
    const void *restrict a,
    const void *restrict b)
{
    if ((*(double *)a) < (*(double *)b))
	return -1;
    if ((*(double *)a) > (*(double *)b))
	return 1;
    return 0;
}

struct qt_qsort_args {
    double *array;
    double pivot;
    size_t length, chunksize, jump, offset;
    aligned_t *restrict furthest_leftwall, *restrict furthest_rightwall;
};

static aligned_t qt_qsort_partition(
    qthread_t * me,
    struct qt_qsort_args *args)
{
    double *a = args->array;
    const double pivot = args->pivot;
    const size_t chunksize = args->chunksize;
    const size_t length = args->length;
    const size_t jump = args->jump;
    size_t leftwall, rightwall;

    leftwall = 0;
    rightwall = length - 1;
    /* adjust the edges; this is critical for this algorithm */
    while (a[leftwall] <= pivot) {
	if ((leftwall + 1) % chunksize != 0) {
	    leftwall++;
	} else {
	    leftwall += jump;
	}
	if (rightwall < leftwall)
	    goto quickexit;
    }
    while (a[rightwall] > pivot) {
	if (rightwall % chunksize != 0) {
	    if (rightwall == 0)
		goto quickexit;
	    rightwall--;
	} else {
	    if (rightwall < jump)
		goto quickexit;
	    rightwall -= jump;
	}
	if (rightwall < leftwall)
	    goto quickexit;
    }
    SWAP(a, leftwall, rightwall);
    while (1) {
	do {
	    leftwall += ((leftwall + 1) % chunksize != 0) ? 1 : jump;
	    if (rightwall < leftwall)
		goto quickexit;
	} while (a[leftwall] <= pivot);
	if (rightwall <= leftwall)
	    break;
	do {
	    if (rightwall % chunksize != 0) {
		if (rightwall == 0)
		    goto quickexit;
		rightwall--;
	    } else {
		if (rightwall < jump)
		    goto quickexit;
		rightwall -= jump;
	    }
	} while (a[rightwall] > pivot);
	if (rightwall <= leftwall)
	    break;
	SWAP(a, leftwall, rightwall);
    }
  quickexit:
    qthread_lock(me, args->furthest_leftwall);
    if (leftwall + args->offset < *args->furthest_leftwall) {
	*args->furthest_leftwall = leftwall + args->offset;
    }
    if (rightwall + args->offset > *args->furthest_rightwall) {
	*args->furthest_rightwall = rightwall + args->offset;
    }
    qthread_unlock(me, args->furthest_leftwall);
    return 0;
}

struct qt_qsort_iargs {
    double *array;
    size_t length;
};

struct qt_qsort_iprets {
    aligned_t leftwall, rightwall;
};

static struct qt_qsort_iprets qt_qsort_inner_partitioner(
    qthread_t * me,
    double *array,
    const size_t length,
    const double pivot)
{				       /*{{{ */
    const size_t chunksize = 10;

    /* choose the number of threads to use */
    const size_t numthreads = qthread_num_shepherds();

    /* a "megachunk" is a set of numthreads chunks.
     * calculate the megachunk information for determining the array lengths
     * each thread will be fed. */
    const size_t megachunk_size = chunksize * numthreads;

    /* just used as a boolean test */
    const size_t extra_chunks = length % megachunk_size;

    /* non-consts */
    size_t megachunks = length / (chunksize * numthreads);
    struct qt_qsort_iprets retval = { ((aligned_t) - 1), 0 };
    syncvar_t *rets;
    struct qt_qsort_args *args;
    size_t i;

    rets = (syncvar_t *) calloc(numthreads, sizeof(syncvar_t));
    args =
	(struct qt_qsort_args *)malloc(sizeof(struct qt_qsort_args) *
				       numthreads);
    /* spawn threads to do the partitioning */
    for (i = 0; i < numthreads; i++) {
	args[i].array = array + (i * chunksize);
	args[i].offset = i * chunksize;
	args[i].pivot = pivot;
	args[i].chunksize = chunksize;
	args[i].jump = (numthreads - 1) * chunksize + 1;
	args[i].furthest_leftwall = &retval.leftwall;
	args[i].furthest_rightwall = &retval.rightwall;
	if (extra_chunks != 0) {
	    args[i].length = megachunks * megachunk_size + chunksize;
	    if (args[i].length + args[i].offset >= length) {
		args[i].length = length - args[i].offset;
		megachunks--;
	    }
	} else {
	    args[i].length = length - megachunk_size + chunksize;
	}
	/* qt_qsort_partition(me, args+i); */
	/* future_fork((qthread_f)qt_qsort_partition, args+i, rets+i); */
	qthread_fork_syncvar((qthread_f) qt_qsort_partition, args + i, rets + i);
    }
    for (i = 0; i < numthreads; i++) {
	qthread_syncvar_readFF(me, NULL, rets + i);
    }
    free(args);
    free(rets);

    return retval;
}				       /*}}} */

static aligned_t qt_qsort_inner(
    qthread_t * me,
    const struct qt_qsort_iargs *a)
{				       /*{{{ */
    const size_t len = a->length;
    double *array = a->array;
    size_t i;
    struct qt_qsort_iprets furthest;
    const size_t thread_chunk = len / qthread_num_shepherds();

    /* choose the number of threads to use */
    if (qthread_num_shepherds() == 1 || len <= 10000) {	/* shortcut */
	qsort(array, len, sizeof(double), dcmp);
	return 0;
    }
    furthest.leftwall = 0;
    furthest.rightwall = len - 1;
    /* tri-median pivot selection */
    i = len / 2;
    if (array[0] > array[i]) {
	SWAP(array, 0, i);
    }
    if (array[0] > array[len - 1]) {
	SWAP(array, 0, len - 1);
    }
    if (array[i] > array[len - 1]) {
	SWAP(array, i, len - 1);
    }
    {
	const double pivot = array[i];

	while (furthest.rightwall > furthest.leftwall &&
	       furthest.rightwall - furthest.leftwall > (2 * thread_chunk)) {
	    const size_t offset = furthest.leftwall;

	    furthest =
		qt_qsort_inner_partitioner(me, array + furthest.leftwall,
					   furthest.rightwall -
					   furthest.leftwall + 1, pivot);
	    furthest.leftwall += offset;
	    furthest.rightwall += offset;
	}
	/* data between furthest.leftwall and furthest.rightwall is unlikely to be partitioned correctly */
	{
	    size_t leftwall = furthest.leftwall, rightwall =
		furthest.rightwall;

	    while (leftwall < rightwall && array[leftwall] <= pivot)
		leftwall++;
	    while (leftwall < rightwall && array[rightwall] > pivot)
		rightwall--;
	    if (leftwall < rightwall) {
		SWAP(array, leftwall, rightwall);
		for (;;) {
		    while (++leftwall < rightwall &&
			   array[leftwall] <= pivot) ;
		    if (rightwall < leftwall)
			break;
		    while (leftwall < --rightwall &&
			   array[rightwall] > pivot) ;
		    if (rightwall < leftwall)
			break;
		    SWAP(array, leftwall, rightwall);
		}
	    }
	    if (array[rightwall] <= pivot) {
		rightwall++;
	    }
	    /* now, spawn the next two iterations */
	    {
		struct qt_qsort_iargs na[2];
		syncvar_t rets[2] = { SYNCVAR_STATIC_INITIALIZER, SYNCVAR_STATIC_INITIALIZER };
		na[0].array = array;
		na[0].length = rightwall;
		na[1].array = array + rightwall;
		na[1].length = len - rightwall;
		if (na[0].length > 0) {
		    /* future_fork((qthread_f)qt_qsort_inner, na, rets); */
		    /* qt_qsort_inner(me, na); */
		    qthread_fork_syncvar((qthread_f) qt_qsort_inner, na, rets);
		}
		if (na[1].length > 0 && len > rightwall) {
		    /* future_fork((qthread_f)qt_qsort_inner, na+1, rets+1); */
		    /* qt_qsort_inner(me, na+1); */
		    qthread_fork_syncvar((qthread_f) qt_qsort_inner, na + 1,
				 rets + 1);
		}
		qthread_syncvar_readFF(me, NULL, rets);
		qthread_syncvar_readFF(me, NULL, rets + 1);
	    }
	}
    }
    return 0;
}				       /*}}} */

void qt_qsort(
    qthread_t * me,
    double *array,
    const size_t length)
{				       /*{{{ */
    struct qt_qsort_iargs arg;

    arg.array = array;
    arg.length = length;

    qt_qsort_inner(me, &arg);
}				       /*}}} */

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
# ifdef __INTEL_COMPILER
/* external function with no prior declaration */
#  pragma warning (disable:1418)
/* external declaration in primary source file */
#  pragma warning (disable:1419)
# endif
/* function calls added to support OpenMP-to-qthreads translation via the ROSE compiler
 *  - also gets the loops in the form preferred by MAESTRO
 *    easier dynamic parallel load balancing
 *    facilitates nested loop handling by allowing shepherd additions after initial loop construction
 */

static int activeParallel = 0;

/* qt_parallel - translator for qt_loop() */
void qt_parallel(
    const qt_loop_step_f func,
    const unsigned int threads,
    void *argptr)
{
    activeParallel = 1;
    qt_loop_step_inner(0, threads, 1, func, argptr, 0);
    activeParallel = 0;
}

/* qt_parallel_for - function generated in response to OpenMP parallel for
 *    - Rose takes contents of the loop an makes an out-of-line function
 *      (func) assigns an iteration number (iter) and generates an argument
 *      list (argptr) - the function is responsible for knowing how long
 *      the argument list is - qthreads is responsible for assigning
 *      iterations numbers and making sure that every loop iteration is
 *      complete before allowing execution to continue in the calling function
 *
 * This is called within parallel regions - every thread calls this function
 * but we only need one parallel loop and the shepherds need to share
 * iterations.  Care is needed to insure only one copy is executed.
 */
int forLockInitialized = 0;
QTHREAD_FASTLOCK_TYPE forLock;	// used for mutual exclusion in qt_parallel_for - needs init
volatile int forLoopsStarted = 0;	// used for active loop in qt_parallel_for
int qthread_forCount(
    qthread_t *,
    int);

// written by AKP
int cnbWorkers;
double cnbTimeMin;

int qloop_internal_computeNextBlock(
    int block,
    double time,
    volatile qtrose_loop_handle_t * loop)
{

    int newBlock;
    int remaining = loop->assignStop - loop->assignNext;
    if (remaining <= 0)
	return 0;

    if (time < 7.5e-7)
	newBlock = block;
    else
	newBlock = (remaining / ((cnbWorkers << 1) * (loop->assignStep))) + 1;

    return newBlock?newBlock:1;
}

void qt_loop_queue_run_single(
    volatile qtrose_loop_handle_t * loop,
    void *t)
{
    qthread_t *const me = qthread_self();
    int dynamicBlock = qloop_internal_computeNextBlock(0, 1.0, loop);	// fake time to prevent blocking for short time chunks
    qtimer_t qt = qtimer_create();
    double time = 0.;

    // get next loop iteration (shared access)
    aligned_t iterationNumber = qthread_incr(&loop->assignNext, dynamicBlock);
    aligned_t iterationStop = iterationNumber + dynamicBlock;
    if (iterationStop > loop->assignStop)
	iterationStop = loop->assignStop;

    while (iterationNumber < loop->assignStop) {
	if (t != NULL) {
	    qtimer_start(qt);
	    loop->func(me, iterationNumber, iterationStop,
			    loop->assignStep, t);
	    qtimer_stop(qt);
	} else {
	    qtimer_start(qt);
	    loop->func(me, iterationNumber, iterationStop,
			    loop->assignStep, loop->arg);
	    qtimer_stop(qt);
	}
	time = qtimer_secs(qt);
	if (time < cnbTimeMin)
	    cnbTimeMin = time;

	dynamicBlock = qloop_internal_computeNextBlock(dynamicBlock, time, loop);

	iterationNumber =
	    (size_t) qthread_incr(&loop->assignNext, dynamicBlock);
	iterationStop = iterationNumber + dynamicBlock;
	if (iterationStop > loop->assignStop)
	    iterationStop = loop->assignStop;
    }

    // did some work in this loop; need to wait for others to finish

    //    qtimer_start(qt);
    qt_global_barrier(me);	       // keep everybody together -- don't lose workers

    //    qtimer_stop(qt);
    //    time = qtimer_secs(qt);
    //    printf("\twait time %e dynamicBlock %d  MinTime %e\n",time, dynamicBlock, cnbTimeMin);

    qtimer_destroy(qt);
    return;
}

volatile qtrose_loop_handle_t *activeLoop = NULL;

static void qt_parallel_qfor(
    const qt_loop_step_f func,
    const size_t startat,
    const size_t stopat,
    const size_t incr,
    void *restrict argptr)
{
    qthread_t *me = qthread_self();
    volatile qtrose_loop_handle_t *qqhandle = NULL;
    int forCount = qthread_forCount(me, 1);	// my loop count

    cnbWorkers = qthread_num_shepherds();
    cnbTimeMin = 1.0;

    QTHREAD_FASTLOCK_LOCK(&forLock);   // KBW: XXX: need to find a way to avoid locking
    if (forLoopsStarted < forCount) {  // is this a new loop? - if so, add work
	qqhandle = qtrose_loop_queue_create(TIMED, startat, stopat, incr, func, argptr);	// put loop on the queue
	forLoopsStarted = forCount;    // set current loop number
	activeLoop = qqhandle;
    } else {
	if (forLoopsStarted != forCount) {	// out of sync
	    QTHREAD_FASTLOCK_UNLOCK(&forLock);
	    return;
	} else {
	    qqhandle = activeLoop;
	}
    }
    QTHREAD_FASTLOCK_UNLOCK(&forLock);

    qt_loop_queue_run_single(qqhandle, argptr);

    return;
}

static void qt_naked_parallel_for(
    qthread_t * me,
    const size_t dummy1,
    const size_t dummy2,
    const size_t dummy3,
    void *nakedArg);

void qt_parallel_for(
    const qt_loop_step_f func,
    const size_t startat,
    const size_t stopat,
    const size_t incr,
    void *restrict argptr)
{
    //  printf("new for stmt \n");
    //  int t = 0;
    //    qtimer_t qt = qtimer_create();
    //    qtimer_start(qt);
    if (forLockInitialized == 0) {
	forLockInitialized = 1;
	QTHREAD_FASTLOCK_INIT(forLock);
    }
    if (!activeParallel) {
      void *nakedArg[5] = { (void *)func, (void*)startat, (void*)stopat,
			    (void*)incr, (void *)argptr };  // pass the loop bounds for the
                                                            // for loop we are wrapping
	activeParallel = 1;
	qt_loop_step_inner(0, qthread_num_shepherds(), 1,
		qt_naked_parallel_for, nakedArg, 0);
	activeParallel = 0;

    } else {
	qt_parallel_qfor(func, startat, stopat, incr, argptr);
    }
    //    qtimer_stop(qt);
    //    double time = qtimer_secs(qt);
    //    printf("for loop %d time %e \n",t,time);
    //    qtimer_destroy(qt);
}

static void qt_naked_parallel_for(
    qthread_t * me,
    const size_t dummy1,                    // This function must match qt_loop_f prototype so it can be called with qt_loop()
    const size_t dummy2,                    // But since I only care when all the children return, the arguments are ignored
    const size_t dummy3,
    void *nakedArg)
{
    void **funcArgs = (void **)nakedArg;
    const qt_loop_step_f func = (const qt_loop_step_f)(funcArgs[0]);
    const size_t startat = (size_t)funcArgs[1]; // get the for loop bounds from the argument
    const size_t stopat = (size_t)funcArgs[2];
    const size_t step = (size_t)funcArgs[3];
    void *restrict argptr = funcArgs[4];
    qt_parallel_qfor(func, startat, stopat, step, argptr);
}
#endif
