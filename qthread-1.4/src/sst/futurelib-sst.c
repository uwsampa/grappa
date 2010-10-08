#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <qthread/qthread.h>
#include <qthread/futurelib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>		       /* for va_start and va_end */

#include <ppcPimCalls.h>

#include "futurelib_innards.h"
#include "qthread_innards.h"

#undef DBprintf
#if 0
static inline void DBprintf(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fflush(stderr);		       /* KBW: helps keep things straight */
}
#else
#define DBprintf(...) ;
#endif

#define INIT_LOCK(qqq,xxx) qthread_unlock(qqq, (void*)(xxx))
#define LOCK(qqq,xxx) qthread_lock(qqq, (void*)(xxx))
#define UNLOCK(qqq,xxx) qthread_unlock(qqq, (void*)(xxx))

#define THREAD_LOC(qqq) qthread_shep(qqq)

#define MALLOC(sss) malloc(sss)
#define FREE(sss) free(sss)

/* global data (copy everywhere) */
location_t * future_bookkeeping_array = NULL;
//static qthread_shepherd_id_t shep_for_new_futures = 0;
static pthread_mutex_t sfnf_lock;

/* This function is critical to futurelib, and as such must be as fast as
 * possible.
 *
 * If the qthread is not a future, it returns NULL; otherwise, it returns a
 * pointer to the bookkeeping structure associated with that future's shepherd.
 */
static inline location_t *ft_loc(qthread_t * ft)
{
    return qthread_isfuture(ft) ? &(future_bookkeeping_array[qthread_shep(NULL)]) : NULL;
}

/* this function is used as a qthread; it is run by each shepherd so that each
 * shepherd will get some PIM-local data associated with it. */
static aligned_t future_shep_init(qthread_t * me, void * arg)
{
    location_t * bk = (location_t *)PIM_localMallocAtID((unsigned int)sizeof(location_t), (unsigned int)arg);
    PIM_mem_region_create( 0 /* I think this argument is ignored... */,
	    future_bookkeeping_array+((unsigned int)arg) /* start address that will be remapped */,
	    sizeof(location_t) /* number of bytes to remap */,
	    bk /* where it will be remapped to */,
	    0 /* this has something to do with cache (?) */);
    return 0;
}

void future_init(int vp_per_loc)
{
    unsigned int i;
    aligned_t *rets;
    qthread_t *me = qthread_self();

    pthread_mutex_init(&sfnf_lock, NULL);
    future_bookkeeping_array =
	(location_t *) MALLOC(sizeof(location_t) * PIM_readSpecial(PIM_CMD_LOC_COUNT));
    rets = (aligned_t *) MALLOC(sizeof(aligned_t) * PIM_readSpecial(PIM_CMD_LOC_COUNT));
    for (i = 0; i < (unsigned int) PIM_readSpecial(PIM_CMD_LOC_COUNT); i++) {
	pthread_mutex_init(&(future_bookkeeping_array[i].vp_count_lock),
		NULL);
	qthread_fork_to(future_shep_init, NULL, rets + i, i);
    }
    for (i = 0; i < (unsigned int) PIM_readSpecial(PIM_CMD_LOC_COUNT); i++) {
	qthread_readFF(me, rets + i, rets + i);
	future_bookkeeping_array[i].vp_count = 0;
	future_bookkeeping_array[i].vp_max = vp_per_loc;
	future_bookkeeping_array[i].id = i;
    }
    FREE(rets);
}

/* This is the heart and soul of the futurelib. This function has two purposes:
 * first, it checks for (and grabs, if it exists) an available thread-execution
 * slot. second, if there is no available slot, it adds itself to the waiter
 * queue to get one.
 */
inline void blocking_vp_incr(qthread_t * me, location_t * loc)
{
    pthread_mutex_lock(&(loc->vp_count_lock));
    DBprintf("Thread %p try blocking increment on loc %d vps %d\n", me,
	    loc->id, loc->vp_count);
    while (loc->vp_count >= loc->vp_max) {
	pthread_mutex_unlock(&(loc->vp_count_lock));
	DBprintf("thread %p found too many futures in %d; waiting for vp_count\n",
		me, loc->id);
	qthread_lock(me, &(loc->vp_count));
	pthread_mutex_lock(&(loc->vp_count_lock));
    }
    loc->vp_count++;
    DBprintf("Thread %p incr loc %d to %d vps\n", me, loc->id, loc->vp_count);
    pthread_mutex_unlock(&(loc->vp_count_lock));
}

/* See the real futurelib.c for documentation
 * */
void future_fork(qthread_f fptr, void *arg, aligned_t * retval)
{
    qthread_shepherd_id_t rr = 0;
    //qthread_t *me = qthread_self();
    //location_t *ptr = ft_loc(me);

    DBprintf("Thread %p forking a future\n", me);
    /* step 1: figure out where to go (fast) */
#if 1
    rr = 0;
#else
    if (ptr) {
	rr = ptr->sched_shep++;
	ptr->sched_shep *= (ptr->sched_shep == (unsigned int) PIM_readSpecial(PIM_CMD_LOC_COUNT));
    } else {
	pthread_mutex_lock(&sfnf_lock);
	rr = shep_for_new_futures++;
	shep_for_new_futures *= (shep_for_new_futures == (unsigned int) PIM_readSpecial(PIM_CMD_LOC_COUNT));
	pthread_mutex_unlock(&sfnf_lock);
    }
#endif
    DBprintf("Thread %p decided future will go to %i\n", me, rr);
    /* Steps 2&3 (slow) */
    future_fork_to(fptr, arg, retval, rr);
}
void future_fork_to(qthread_f fptr, void *arg, aligned_t * retval, qthread_shepherd_id_t rr)
{
    qthread_t *me = qthread_self();
    blocking_vp_incr(me, &(future_bookkeeping_array[rr]));
    qthread_fork_future_to(me, fptr, arg, retval, rr);
}

/* This function declares that the current function (qthr) no longer
 * counts toward the per-shepherd resource restrictions
 */
int future_yield(qthread_t * me)
{
    location_t *loc = ft_loc(me);

    DBprintf("Thread %p yield on loc %p\n", me, loc);
    /* Non-futures do not have a vproc to yield */
    if (loc != NULL) {
	char unlockit;
	/* yield vproc */
	DBprintf("Thread %p yield loc %i vps %d\n", me, loc->id,
		 loc->vp_count);
	pthread_mutex_lock(&(loc->vp_count_lock));
	unlockit = (loc->vp_count-- == loc->vp_max);
	pthread_mutex_unlock(&(loc->vp_count_lock));
	if (unlockit) {
	    qthread_unlock(me, &(loc->vp_count));
	}
	return 1;
    }
    return 0;
}

void future_acquire(qthread_t * me)
{
    location_t *loc = ft_loc(me);

    DBprintf("Thread %p acquire on loc %p\n", me, loc);
    /* Non-futures need not acquire a v proc */
    if (loc != NULL) {
	blocking_vp_incr(me, loc);
    }
}

static void future_join(qthread_t * me, aligned_t * ft)
{
    DBprintf("Qthread %p join to future %p\n", me, ft);
    qthread_readFF(me, ft, ft);
}

void future_exit(qthread_t * me)
{
    DBprintf("Thread %p exit on loc %d\n", me, THREAD_LOC(me));
    future_yield(me);
    qthread_assertnotfuture(me);
}

void future_join_all(qthread_t * me, aligned_t * fta, int ftc)
{
    int i;

    DBprintf("Qthread %p join all to %d futures\n", me, ftc);
    for (i = 0; i < ftc; i++)
	future_join(me, fta+i);
}
