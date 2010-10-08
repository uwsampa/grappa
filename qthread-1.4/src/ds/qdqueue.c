#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdlib.h>		       /* for calloc() */
#include <limits.h>		       /* for INT_MAX, per C89 */
#include <qthread/qthread.h>
#include <qthread/qlfqueue.h>
#include <qthread/qdqueue.h>
#include "qthread_asserts.h"
#ifdef HAVE_SYS_LGRP_USER_H
# include <sys/lgrp_user.h>
#endif
#ifdef QTHREAD_HAVE_LIBNUMA
# include <numa.h>
#endif

struct qdsubqueue_s;

struct qdqueue_adstruct_s {
    struct qdsubqueue_s *shep;
    aligned_t generation;
};

struct qdqueue_adheap_elem_s {
    int inheap;
    struct qdqueue_adstruct_s ad;
    struct qdqueue_adheap_elem_s *prev;
    struct qdqueue_adheap_elem_s *next;
};

struct qdqueue_adheap_s {
    aligned_t gateway_lock;	/* unnecessary, but avoids compiler warnings about punning */
    struct qdqueue_adheap_elem_s *first;
    struct qdqueue_adheap_elem_s *heap;
};

struct qdsubqueue_s {
    qlfqueue_t *theQ;
    struct qdsubqueue_s *volatile last_consumed;

    struct qdqueue_adheap_s ads;
    aligned_t last_ad_issued;
    aligned_t last_ad_consumed;

    size_t nNeighbors;
    struct qdsubqueue_s **neighbors;	/* ordered by distance */
    struct qdsubqueue_s **allsheps;	/* ordered by distance */
};

struct qdqueue_s {
    struct qdsubqueue_s *Qs;
};

static qthread_shepherd_id_t maxsheps = 0;

/* avoid compiler bugs with volatile... */
static Q_NOINLINE struct qdsubqueue_s *volatile *vol_id_qdsqptr(struct
								qdsubqueue_s
								*volatile
								*ptr)
{				       /*{{{ */
    return ptr;
}				       /*}}} */

#define _(x) *vol_id_qdsqptr(&(x))

static struct qdqueue_adstruct_s qdqueue_adheap_pop(qthread_t * me, struct qdqueue_adheap_s
						    *heap)
{				       /*{{{ */
    struct qdqueue_adstruct_s ret;

    if (heap->first != NULL) {
	struct qdqueue_adheap_elem_s *tmp;

	qthread_lock(me, &(heap->gateway_lock));
	/* pull off "first" */
	if ((tmp = heap->first) == NULL) {
	    qthread_unlock(me, &(heap->gateway_lock));
	    goto emptyheap;
	}
	ret = tmp->ad;
	if ((heap->first = tmp->next) != NULL) {
	    heap->first->prev = NULL;
	}
	tmp->inheap = 0;
	qthread_unlock(me, &(heap->gateway_lock));
    } else {
      emptyheap:
	ret.shep = NULL;
	ret.generation = 0;
    }
    return ret;
}				       /*}}} */

/* only push if there are waiters...
 * this function is only used if we can *block* for waiting (not implemented yet) */
/*static void qdqueue_adheap_pushcond(qlfqueue_t * heap, void *shep, aligned_t gen)
{
    printf("pushcond\n");
}*/

static void qdqueue_adheap_push(qthread_t * me, struct qdqueue_adheap_s *heap,
				void *shep, aligned_t gen)
{				       /*{{{ */
    size_t i;

    /* search through the heap for a ptr matching shep */
    for (i = 0; i < maxsheps; i++) {
	if (heap->heap[i].ad.shep == shep) {
	    break;
	}
    }
    assert(heap->heap[i].ad.shep == shep);
    qthread_lock(me, &(heap->gateway_lock));
    /* update the gen record */
    if (heap->heap[i].ad.generation < gen || gen == 0) {
	if (gen != 0) {
	    heap->heap[i].ad.generation = gen;
	}
	if (heap->heap[i].inheap == 1) {
	    goto done_pushing;
	}
	heap->heap[i].inheap = 1;
	if (heap->first == NULL) {
	    heap->heap[i].prev = NULL;
	    heap->heap[i].next = NULL;
	    heap->first = &(heap->heap[i]);
	} else {
	    if (&(heap->heap[i]) < heap->first) {
		/* adding *before* the first */
		heap->heap[i].next = heap->first;
		heap->heap[i].prev = NULL;
		heap->first->prev = &(heap->heap[i]);
		heap->first = &(heap->heap[i]);
	    } else if (&(heap->heap[i]) > heap->first) {
		size_t j;

		/* adding somewhere *after* the first
		 * ... so we start here and work backwards until we find some part of the heap */
		for (j = i - 1;; j--) {
		    if (heap->heap[j].inheap) {
			heap->heap[i].next = heap->heap[j].next;
			heap->heap[i].prev = &(heap->heap[j]);
			heap->heap[j].next = &(heap->heap[i]);
			if (heap->heap[i].next != NULL) {
			    heap->heap[i].next->prev = heap->heap + i;
			}
			break;
		    }
		}
	    }			       /* else it WAS the first already, and got re-pushed */
	}
    }
  done_pushing:
    qthread_unlock(me, &(heap->gateway_lock));
}				       /*}}} */

static int qdqueue_adheap_empty(struct qdqueue_adheap_s *heap)
{				       /*{{{ */
    return (heap->first == NULL);
}				       /*}}} */

static void qdqueue_internal_gensheparray(int **a)
{				       /*{{{ */
    qthread_shepherd_id_t i, j;

    for (i = 0; i < maxsheps; i++) {
	for (j = 0; j < maxsheps; j++) {
	    a[i][j] = qthread_distance(i, j);
	}
    }
}				       /*}}} */

static void qdqueue_internal_sortedsheps(qthread_shepherd_id_t shep,
					 struct qdsubqueue_s **all,
					 struct qdsubqueue_s *Qs,
					 const int *distances)
{				       /*{{{ */
    qthread_shepherd_id_t i;
    int lastdist = INT_MIN, maxdist = 0;

    for (i = 0; i < maxsheps; i++) {
	if (maxdist <= distances[i])
	    maxdist = distances[i] + 1;
    }

    i = 0;
    while (i < (maxsheps - 1)) {       /* -1 because we're not recording *self* */
	qthread_shepherd_id_t j;
	int mindist = maxdist;
	size_t count = 0, k;
	qthread_shepherd_id_t *thisdist;

	/* first, find the next smallest distance (bigger than lastdist) */
	for (j = 0; j < maxsheps; j++) {
	    if (j == shep)
		continue;
	    if (distances[j] > lastdist && distances[j] < mindist) {
		mindist = distances[j];
		count = 1;
	    } else if (distances[j] == mindist) {
		count++;
	    }
	}
	lastdist = mindist;
	/* now, create an array of all the sheps that are exactly that far away */
	thisdist = malloc(count * sizeof(qthread_shepherd_id_t));
	assert(thisdist);
	for (j = 0, k = 0; j < maxsheps && k < count; j++) {
	    if (j == shep)
		continue;
	    if (distances[j] == mindist) {
		thisdist[k++] = j;
	    }
	}
	/* and now randomly append them to the all array */
	for (j = 0; j < count; j++) {
	    size_t randpick = random() % (count - j);

	    all[i++] = &(Qs[thisdist[randpick]]);
	    for (k = randpick; k < (count - j - 1); k++) {
		thisdist[k] = thisdist[k + 1];
	    }
	}
	free(thisdist);
    }
}				       /*}}} */

static struct qdsubqueue_s
           **qdqueue_internal_getneighbors(qthread_shepherd_id_t shep,
					   struct qdsubqueue_s *Qs,
					   size_t * numNeighbors,
					   int *Q_UNUSED distances)
{				       /*{{{ */
    struct qdsubqueue_s **ret;
    size_t i;

#if (defined(HAVE_SYS_LGRP_USER_H) || defined(QTHREAD_HAVE_LIBNUMA))
    /* we define a "neighbor" in terms of distance, because that's all we've
     * got, so, we find the shortest non-me distance (i.e. >10), and anything
     * that's less than 10 away from that is considered one of my neighbors */
    qthread_shepherd_id_t *temp;
    size_t j, numN = 0;

# ifdef HAVE_SYS_LGRP_USER_H
#  define NONME_DIST 5
# else
#  define NONME_DIST 10
# endif

    /* find the smallest non-me distance */
    int mindist = INT_MAX;

    for (i = 0; i < maxsheps; i++) {
	if (i == shep)
	    continue;
	if (mindist > distances[i]) {
	    mindist = distances[i];
	}
    }
    /* count how many neighbors that works out to */
    for (i = 0; i < maxsheps; i++) {
	if (i == shep)
	    continue;
	if (distances[i] < (mindist + NONME_DIST)) {
	    numN++;
	}
    }
    /* get a list of those neighbors */
    temp = malloc(numN * sizeof(qthread_shepherd_id_t));
    assert(temp);
    for (i = j = 0; i < maxsheps; i++) {
	if (i == shep)
	    continue;
	if (distances[i] < (mindist + NONME_DIST)) {
	    temp[j++] = (qthread_shepherd_id_t) i;
	}
    }
    /* and now randomly append them to the list */
    ret = malloc(numN * sizeof(struct qdsubqueue_s *));
    assert(ret);
    for (i = 0; i < numN; i++) {
	size_t k, randpick = random() % (numN - i);

	ret[i] = &(Qs[temp[randpick]]);
	for (k = randpick; k < (numN - i - 1); k++) {
	    temp[k] = temp[k + 1];
	}
    }
    free(temp);
    *numNeighbors = numN;
#else
    /* without a valid NUMA library, *ALL* shepherds are "neighbors" */
    size_t j;

    *numNeighbors = maxsheps - 1;
    ret = malloc(*numNeighbors * sizeof(struct qdsubqueue_s *));
    for (i = j = 0; i < maxsheps; i++) {
	if (i == shep)
	    continue;
	ret[j++] = &(Qs[i]);
    }
#endif
    return ret;
}				       /*}}} */

/* Create a new qdqueue */
qdqueue_t *qdqueue_create(void)
{				       /*{{{ */
    qdqueue_t *ret;
    qthread_shepherd_id_t curshep;
    int **sheparray;

    if (maxsheps == 0)
	maxsheps = qthread_num_shepherds();
    ret = calloc(1, sizeof(struct qdqueue_s));
    qassert_goto((ret != NULL), erralloc_killq);
    ret->Qs = calloc(maxsheps, sizeof(struct qdsubqueue_s));

    sheparray = malloc(maxsheps * sizeof(int *));
    qassert_goto((sheparray != NULL), erralloc_killq);
    for (curshep = 0; curshep < maxsheps; curshep++) {
	sheparray[curshep] = malloc(maxsheps * sizeof(int));
	qassert_goto((sheparray[curshep] != NULL), erralloc_killshep);
    }
    qdqueue_internal_gensheparray(sheparray);
    for (curshep = 0; curshep < maxsheps; curshep++) {
	ret->Qs[curshep].theQ = qlfqueue_create();
	ret->Qs[curshep].last_ad_issued = 1;
	ret->Qs[curshep].last_ad_consumed = 1;
	ret->Qs[curshep].allsheps =
	    calloc((maxsheps - 1), sizeof(struct qdsubqueue_s *));
	/* yes, I could get this information from qthreads, but I'm adding a
	 * little bit of randomnes to the list when the distances are equal */
	qdqueue_internal_sortedsheps(curshep, ret->Qs[curshep].allsheps,
				     ret->Qs, sheparray[curshep]);
	ret->Qs[curshep].neighbors =
	    qdqueue_internal_getneighbors(curshep, ret->Qs,
					  &(ret->Qs[curshep].nNeighbors),
					  sheparray[curshep]);
	ret->Qs[curshep].ads.heap =
	    calloc(maxsheps, sizeof(struct qdqueue_adheap_elem_s));
	ret->Qs[curshep].ads.heap[0].ad.shep = &(ret->Qs[curshep]);
	for (qthread_shepherd_id_t i = 0; i < (maxsheps - 1); i++) {
	    ret->Qs[curshep].ads.heap[i + 1].ad.shep =
		ret->Qs[curshep].allsheps[i];
	}
	ret->Qs[curshep].ads.first = NULL;
    }
    for (curshep = 0; curshep < maxsheps; curshep++) {
	free(sheparray[curshep]);
    }
    free(sheparray);

    return ret;

    qgoto(erralloc_killshep);
    for (curshep--; curshep > 0; curshep--) {
	free(sheparray[curshep]);
    }
    free(sheparray);
    qgoto(erralloc_killq);
    if (ret) {
	if (ret->Qs) {
	    free(ret->Qs);
	}
	free(ret);
    }
    return NULL;
}				       /*}}} */

/* destroy that queue */
int qdqueue_destroy(qthread_t * me, qdqueue_t * q)
{				       /*{{{ */
    qthread_shepherd_id_t i;

    qassert_ret((q != NULL), QTHREAD_BADARGS);
    qassert_ret((me != NULL), QTHREAD_BADARGS);
    for (i = 0; i < maxsheps; i++) {
	if (q->Qs[i].theQ) {
	    qlfqueue_destroy(me, q->Qs[i].theQ);
	}
	if (q->Qs[i].ads.heap != NULL) {
	    free(q->Qs[i].ads.heap);
	}
	if (q->Qs[i].neighbors != NULL) {
	    free(q->Qs[i].neighbors);
	}
	if (q->Qs[i].allsheps != NULL) {
	    free(q->Qs[i].allsheps);
	}
    }
    free(q->Qs);
    free(q);
    return QTHREAD_SUCCESS;
}				       /*}}} */

/* enqueue something in the queue */
int qdqueue_enqueue(qthread_t * me, qdqueue_t * q, void *elem)
{				       /*{{{ */
    int stat;
    struct qdsubqueue_s *myq;

    qassert_ret((me != NULL), QTHREAD_BADARGS);
    qassert_ret((q != NULL), QTHREAD_BADARGS);
    qassert_ret((elem != NULL), QTHREAD_BADARGS);

    myq = &(q->Qs[qthread_shep(me)]);

    stat = qlfqueue_empty(myq->theQ);
    qlfqueue_enqueue(me, myq->theQ, elem);
    if (stat) {
	/* the queue was empty, so we may have to wake up waiters */
	/*qdqueue_adheap_pushcond(myq->ads, myq, 0); */
    } else {
	aligned_t generation;
	qthread_shepherd_id_t shep;

	/* the queue had stuff in it already, so we advertise */
	if (myq->last_ad_issued <= myq->last_ad_consumed) {
	    /* only advertise if our existing ads are stale */
	    generation = qthread_incr(&(myq->last_ad_issued), 1);
	    for (shep = 0; shep < myq->nNeighbors; shep++) {
		struct qdsubqueue_s *neighbor = myq->neighbors[shep];

		qdqueue_adheap_push(me, &(neighbor->ads), myq, generation);
	    }
	}
    }

    return QTHREAD_SUCCESS;
}				       /*}}} */

/* enqueue something in the queue at a given location */
int qdqueue_enqueue_there(qthread_t * me, qdqueue_t * q, void *elem,
			  qthread_shepherd_id_t there)
{				       /*{{{ */
    int stat;
    struct qdsubqueue_s *myq;

    qassert_ret((me != NULL), QTHREAD_BADARGS);
    qassert_ret((q != NULL), QTHREAD_BADARGS);
    qassert_ret((elem != NULL), QTHREAD_BADARGS);
    qassert_ret((there < qthread_num_shepherds()), QTHREAD_BADARGS);

    myq = &(q->Qs[there]);

    stat = qlfqueue_empty(myq->theQ);
    qlfqueue_enqueue(me, myq->theQ, elem);
    if (stat) {
	/* the queue was empty, so we may have to wake up waiters */
	/* qdqueue_adheap_pushcond(myq->ads, myq, 0); */
    } else {
	aligned_t generation;
	qthread_shepherd_id_t shep;

	/* the queue had stuff in it already, so we advertise */
	if (myq->last_ad_issued <= myq->last_ad_consumed) {
	    /* only advertise if our existing ads are stale */
	    generation = qthread_incr(&(myq->last_ad_issued), 1);
	    for (shep = 0; shep < myq->nNeighbors; shep++) {
		struct qdsubqueue_s *neighbor = myq->neighbors[shep];

		qdqueue_adheap_push(me, &(neighbor->ads), myq, generation);
	    }
	}
    }

    return QTHREAD_SUCCESS;
}				       /*}}} */

/* dequeue something from the queue (returns NULL for an empty queue) */
void *qdqueue_dequeue(qthread_t * me, qdqueue_t * q)
{				       /*{{{ */
    struct qdsubqueue_s *myq;
    void *ret;

    qassert_ret((me != NULL), NULL);
    qassert_ret((q != NULL), NULL);

    myq = &(q->Qs[qthread_shep(me)]);
    if ((ret = qlfqueue_dequeue(me, myq->theQ)) != NULL) {
	/* this write MUST be atomic */
	_(myq->last_consumed) = myq;
	return ret;
    } else {
	/* this write MUST be atomic */
	_(myq->last_consumed) = NULL;
      checkads:
	{
	    struct qdqueue_adstruct_s ad;

	    while ((ad = qdqueue_adheap_pop(me, &myq->ads)).shep != NULL) {
		struct qdsubqueue_s *lc = _(ad.shep->last_consumed);

		if (lc == ad.shep) {
		    /* it's working on its own queue */
		    aligned_t last_ad = ad.shep->last_ad_consumed;

		    while (last_ad < ad.generation) {
			last_ad =
			    qthread_cas(&(ad.shep->last_ad_consumed), last_ad,
					ad.generation);
		    }
		    if ((ret = qlfqueue_dequeue(me, ad.shep->theQ)) != NULL) {
			_(myq->last_consumed) = ad.shep;
			return ret;
		    }
		} else if (lc != NULL) {
		    /* it got work from somewhere! */
		    qdqueue_adheap_push(me, &myq->ads, lc, 0);
		    /* reset the remote host's last_consumed counter, to avoid infinite loops */
		    (void)qthread_cas_ptr((void*volatile*)&(ad.shep->last_consumed),
					  (void *)lc, NULL);
		}
	    }
	}
	{
	    qthread_shepherd_id_t shep;

	    for (shep = 0; shep < (maxsheps - 1); shep++) {
		struct qdsubqueue_s *remoteshep = myq->allsheps[shep];
		struct qdsubqueue_s *lc = _(remoteshep->last_consumed);

		if ((ret = qlfqueue_dequeue(me, remoteshep->theQ)) != NULL) {
		    _(myq->last_consumed) = remoteshep;
		    return ret;
		} else if (lc != NULL && lc != remoteshep) {
		    /* it got work from somewhere! */
		    if ((ret = qlfqueue_dequeue(me, lc->theQ)) != NULL) {
			_(myq->last_consumed) = lc;
			return ret;
		    }
		}
		if (!qdqueue_adheap_empty(&myq->ads))
		    goto checkads;
	    }
	}
	return NULL;
    }
}				       /*}}} */

/* returns 1 if the queue is empty, 0 otherwise */
int qdqueue_empty(qthread_t * me, qdqueue_t * q)
{				       /*{{{ */
    struct qdsubqueue_s *myq;

    assert(me);
    assert(q);
    if (!me || !q) {
	return 0;
    }

    myq = &(q->Qs[qthread_shep(me)]);
    if (!qlfqueue_empty(myq->theQ)) {
	return 0;
    } else {
	qthread_shepherd_id_t shep;

	for (shep = 0; shep < (maxsheps - 1); shep++) {
	    struct qdsubqueue_s *remoteshep = myq->allsheps[shep];

	    assert(remoteshep);
	    if (_(remoteshep->last_consumed) == remoteshep) {
		/* it's working on its own queue */
		if (!qlfqueue_empty(remoteshep->theQ)) {
		    return 0;
		}
	    }
	}
	return 1;		       /* we searched everywhere, and every queue was empty */
    }
}				       /*}}} */
