#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>		       /* debugging */
#include <qthread/qthread.h>
#include <qthread/qpool.h>
#include <stddef.h>		       /* for size_t (according to C89) */
#include <stdlib.h>		       /* for calloc() and malloc() */
#if (HAVE_MEMALIGN && HAVE_MALLOC_H)
#include <malloc.h>		       /* for memalign() */
#endif
#ifdef QTHREAD_HAVE_LIBNUMA
# include <numa.h>
# define LIBNUMA_ONLY_ARG(x) x,
# define LIBNUMA_ONLY(x) x
#else
# define LIBNUMA_ONLY_ARG(x)
# define LIBNUMA_ONLY(x)
#endif
#ifdef QTHREAD_USE_VALGRIND
# include <valgrind/memcheck.h>
#else
# define VALGRIND_DESTROY_MEMPOOL(a)
# define VALGRIND_MAKE_MEM_NOACCESS(a, b)
# define VALGRIND_MAKE_MEM_DEFINED(a, b)
# define VALGRIND_CREATE_MEMPOOL(a, b, c)
# define VALGRIND_MEMPOOL_ALLOC(a, b, c)
# define VALGRIND_MEMPOOL_FREE(a, b)
#endif

#include "qthread_innards.h"	       /* for QTHREAD_NO_NODE */
#include "qthread_asserts.h"
#include "qt_gcd.h"		       /* for qt_lcm() */

#ifdef HAVE_GETPAGESIZE
#include <unistd.h>
#else
# define getpagesize() 4096
#endif

struct qpool_shepspec_s {
    void *volatile reuse_pool;
    char *alloc_block;
    size_t alloc_block_pos;
    void **alloc_list;
    size_t alloc_list_pos;

    LIBNUMA_ONLY(unsigned int node;)
    aligned_t lock;
};

struct qpool_s {
    size_t item_size;
    size_t alloc_size;
    size_t items_per_alloc;
    size_t alignment;

    struct qpool_shepspec_s *pools;
};

/* local constants */
static size_t pagesize = 0;

/* avoid compiler bugs with volatile... */
static Q_NOINLINE void *volatile *vol_id_void(void *volatile
						       *const ptr)
{				       /*{{{ */
    return ptr;
}				       /*}}} */

#define _(x) (*vol_id_void(&(x)))

/* local funcs */
static QINLINE void *qpool_internal_aligned_alloc(size_t alloc_size,
						  LIBNUMA_ONLY_ARG(unsigned int Q_UNUSED node)
						  size_t alignment)
{				       /*{{{ */
    void *ret;

#ifdef QTHREAD_HAVE_LIBNUMA
    if (node != QTHREAD_NO_NODE) {     /* guaranteed page alignment */
	ret = numa_alloc_onnode(alloc_size, node);
    } else
#endif
	switch (alignment) {
	    case 0:
		return calloc(1, alloc_size);
	    case 16:
	    case 8:
	    case 4:
	    case 2:
#if defined(HAVE_MEMALIGN)
		ret = memalign(16, alloc_size);
#elif defined(HAVE_POSIX_MEMALIGN)
		posix_memalign(&(ret), 16, alloc_size);
#elif defined(HAVE_WORKING_VALLOC)
		ret = valloc(alloc_size);
#elif defined(HAVE_16ALIGNED_CALLOC)
		return calloc(1, alloc_size);
#elif defined(HAVE_16ALIGNED_MALLOC)
		ret = malloc(alloc_size);
#elif defined(HAVE_PAGE_ALIGNED_MALLOC)
		ret = malloc(alloc_size);
#else
		ret = valloc(alloc_size);	/* cross your fingers */
#endif
		break;
	    default:
#ifdef HAVE_MEMALIGN
		ret = memalign(alignment, alloc_size);
#elif defined(HAVE_POSIX_MEMALIGN)
		posix_memalign(&(ret), alignment, alloc_size);
#elif defined(HAVE_WORKING_VALLOC)
		ret = valloc(alloc_size);
#elif defined(HAVE_PAGE_ALIGNED_MALLOC)
		ret = malloc(alloc_size);
#else
		ret = valloc(alloc_size);	/* cross your fingers */
#endif
	}
    memset(ret, 0, alloc_size);
    return ret;
}				       /*}}} */

static QINLINE void qpool_internal_aligned_free(void *freeme,
						const size_t Q_UNUSED
						alloc_size,
						LIBNUMA_ONLY_ARG(const unsigned int node)
						const size_t Q_UNUSED
						alignment)
{				       /*{{{ */
#ifdef QTHREAD_HAVE_LIBNUMA
    if (node != QTHREAD_NO_NODE) {
	numa_free(freeme, alloc_size);
	return;
    }
#endif
    switch (alignment) {
	case 0:
	    free(freeme);
	    return;
	default:
#if defined(HAVE_16_ALIGNED_CALLOC) || \
	    defined(HAVE_16_ALIGNED_MALLOC) || \
	    defined(HAVE_MEMALIGN) || \
	    defined(HAVE_PAGE_ALIGNED_MALLOC) || \
	    defined(HAVE_POSIX_MEMALIGN) || \
	    defined(HAVE_WORKING_VALLOC)
	    free(freeme);
#endif
	    return;
    }
}				       /*}}} */

// item_size is how many bytes to return
// ...memory is always allocated in multiples of getpagesize()
qpool *qpool_create_aligned(const size_t isize, size_t alignment)
{				       /*{{{ */
    qpool *pool;
    size_t item_size = isize;
    size_t alloc_size = 0;
    const size_t numsheps = qthread_num_shepherds();
    qthread_shepherd_id_t pindex;

    pool = (qpool *) calloc(1, sizeof(struct qpool_s));
    qassert_ret((pool != NULL), NULL);
    VALGRIND_CREATE_MEMPOOL(pool, 0, 0);
    pool->pools =
	(struct qpool_shepspec_s *)calloc(numsheps,
					  sizeof(struct qpool_shepspec_s));
    qassert_goto((pool->pools != NULL), errexit_killpool);
    if (pagesize == 0) {
	pagesize = getpagesize();
    }
    /* first, we ensure that item_size is at least sizeof(void*), and also that
     * it is a multiple of sizeof(void*). The second condition technically
     * implies the first, but it's not a big deal. */
    if (item_size < sizeof(void *)) {
	item_size = sizeof(void *);
    }
    if (item_size % sizeof(void *)) {
	item_size += (sizeof(void *)) - (item_size % sizeof(void *));
    }
    if (alignment <= 16) {
	alignment = 16;
    }
    if (item_size % alignment) {
	item_size += alignment - (item_size % alignment);
    }
    pool->item_size = item_size;
    pool->alignment = alignment;
    /* next, we find the least-common-multiple in sizes between item_size and
     * pagesize. If this is less than ten items (an arbitrary number), we
     * increase the alloc_size until it is at least that big. This guarantees
     * that the allocation size will be a multiple of pagesize (fast!
     * efficient!). */
    alloc_size = qt_lcm(item_size, pagesize);
    if (alloc_size == 0) {	       /* degenerative case */
	if (item_size > pagesize) {
	    alloc_size = item_size;
	} else {
	    alloc_size = pagesize;
	}
    } else {
	while (alloc_size / item_size < 1024) {
	    alloc_size *= 2;
	}
	while (alloc_size < pagesize * 16) {
	    alloc_size *= 2;
	}
    }
    pool->alloc_size = alloc_size;

    pool->items_per_alloc = alloc_size / item_size;

    /* now that we have all the information, set up the pools */
    for (pindex = 0; pindex < numsheps; pindex++) {
	LIBNUMA_ONLY(pool->pools[pindex].node = qthread_internal_shep_to_node(pindex));
	_(pool->pools[pindex].reuse_pool) = NULL;
	pool->pools[pindex].alloc_block =
	    (char *)qpool_internal_aligned_alloc(alloc_size,
						 LIBNUMA_ONLY_ARG(pool->pools[pindex].node)
						 alignment);
	VALGRIND_MAKE_MEM_NOACCESS(pool->pools[pindex].alloc_block,
				   alloc_size);
	qassert_goto((pool->pools[pindex].alloc_block != NULL), errexit_killpool);
	qassert_goto((((unsigned long)(pool->pools[pindex].
				alloc_block) & (alignment - 1)) == 0), errexit_killpool);
	/* this assumes that pagesize is a multiple of sizeof(void*) */
	pool->pools[pindex].alloc_list = calloc(1, pagesize);
	qassert_goto((pool->pools[pindex].alloc_list != NULL), errexit_killpool);
	pool->pools[pindex].alloc_list[0] = pool->pools[pindex].alloc_block;
	pool->pools[pindex].alloc_list_pos = 1;
    }
    return pool;
    qgoto(errexit_killpool);
    if (pool) {
	if (pool->pools) {
	    size_t i;

	    for (i = 0; i < numsheps; i++) {
		if (pool->pools[i].alloc_block) {
		    qpool_internal_aligned_free(pool->pools[i].alloc_block,
						alloc_size,
						LIBNUMA_ONLY_ARG(pool->pools[i].node)
						alignment);
		}
	    }
	    free(pool->pools);
	}
	free(pool);
    }
    return NULL;
}				       /*}}} */

qpool *qpool_create(const size_t item_size)
{				       /*{{{ */
    return qpool_create_aligned(item_size, 0);
}				       /*}}} */

/* to avoid ABA reinsertion trouble, each pointer in the pool needs to have a
 * monotonically increasing counter associated with it. The counter doesn't
 * need to be huge, just big enough to make the APA problem sufficiently
 * unlikely. We'll just claim 4, to be conservative. Thus, an allocated block
 * of memory must be aligned to 16 bytes. */
#if defined(QTHREAD_USE_VALGRIND) && NO_ABA_PROTECTION
# define QPTR(x) (x)
# define QCTR(x) 0
# define QCOMPOSE(x,y) (x)
#else
# define QCTR_MASK (15)
# define QPTR(x) ((void*)(((uintptr_t)(x))&~(uintptr_t)QCTR_MASK))
# define QCTR(x) (((uintptr_t)(x))&QCTR_MASK)
# define QCOMPOSE(x,y) (void*)(((uintptr_t)QPTR(x))|((QCTR(y)+1)&QCTR_MASK))
#endif

void *qpool_alloc(qthread_t * me, qpool * pool)
{				       /*{{{ */
    void *p;
    qthread_shepherd_id_t shep;
    struct qpool_shepspec_s *mypool;

    qassert_ret((pool != NULL), NULL);
    shep = qthread_shep(me);
    mypool = &(pool->pools[shep]);
    p = (void *)_(mypool->reuse_pool);

    if (QPTR(p) != NULL) {
	void *old, *new;

	do {
	    old = p;
	    VALGRIND_MAKE_MEM_DEFINED(QPTR(p), pool->item_size);
	    new = *(void **)QPTR(p);
	    p = (void *)qthread_cas_ptr(&(mypool->reuse_pool), old,
					QCOMPOSE(new, p));
	} while (p != old && QPTR(p) != NULL);
    }
    if (QPTR(p) == NULL) {	       /* this is not an else on purpose */
	/* XXX: *SHOULD* pull from other pools that are "nearby" - at minimum, any
	 * pools from the same node, but different shepherd */

	/* on a single-threaded shepherd, locking is unnecessary */
#ifdef QTHREAD_SST_PRIMITIVES
	qassert(qthread_lock(me, &mypool->lock), 0);
#endif
	if (mypool->alloc_block_pos == pool->items_per_alloc) {
	    if (mypool->alloc_list_pos == (pagesize / sizeof(void *) - 1)) {
		void **tmp = calloc(1, pagesize);

		qassert_goto((tmp != NULL), alloc_exit);
		tmp[pagesize / sizeof(void *) - 1] = mypool->alloc_list;
		mypool->alloc_list = tmp;
		mypool->alloc_list_pos = 0;
	    }
	    p = qpool_internal_aligned_alloc(pool->alloc_size,
					     LIBNUMA_ONLY_ARG(mypool->node)
					     pool->alignment);
	    VALGRIND_MAKE_MEM_NOACCESS(p, pool->alloc_size);
	    qassert_goto((p != NULL), alloc_exit);
	    assert(pool->alignment == 0 ||
		   (((unsigned long)p) & (pool->alignment - 1)) == 0);
	    assert(QCTR(p) == 0);
	    mypool->alloc_block = p;
	    mypool->alloc_block_pos = 1;
	    mypool->alloc_list[mypool->alloc_list_pos] = mypool->alloc_block;
	    mypool->alloc_list_pos++;
	} else {
	    p = mypool->alloc_block +
		(pool->item_size * mypool->alloc_block_pos);
	    mypool->alloc_block_pos++;
	}
#ifdef QTHREAD_SST_PRIMITIVES
	qassert(qthread_unlock(me, &mypool->lock), 0);
#endif
	if (pool->alignment != 0 &&
	    (((unsigned long)p) & (pool->alignment - 1))) {
	    printf("alloc_block = %p\n", mypool->alloc_block);
	    printf("item_size = %u\n", (unsigned)(pool->item_size));
	    assert(pool->alignment == 0 ||
		   (((unsigned long)p) & (pool->alignment - 1)) == 0);
	}
    }
    qgoto(alloc_exit);
    VALGRIND_MEMPOOL_ALLOC(pool, QPTR(p), pool->item_size);
    return QPTR(p);
}				       /*}}} */

void qpool_free(qthread_t * me, qpool * pool, void *mem)
{				       /*{{{ */
    void *p, *old, *new;
    qthread_shepherd_id_t shep = qthread_shep(me);
    struct qpool_shepspec_s *mypool = &(pool->pools[shep]);

    qassert_retvoid((mem != NULL));
    qassert_retvoid((pool != NULL));
    do {
	old = (void *)_(mypool->reuse_pool);	// should be an atomic read
	*(void **)mem = old;
	new = QCOMPOSE(mem, old);
	p = (void *)qthread_cas_ptr(&(mypool->reuse_pool), old, new);
    } while (p != old);
    VALGRIND_MEMPOOL_FREE(pool, mem);
}				       /*}}} */

void qpool_destroy(qpool * pool)
{				       /*{{{ */
    qassert_retvoid((pool != NULL));
    qthread_shepherd_id_t max = qthread_num_shepherds();
    qthread_shepherd_id_t i;

    for (i = 0; i < max; i++) {
	struct qpool_shepspec_s *mypool = &(pool->pools[i]);

	while (mypool->alloc_list) {
	    unsigned int j = 0;

	    void *p = mypool->alloc_list[0];

	    while (p && j < (pagesize / sizeof(void *) - 1)) {
		qpool_internal_aligned_free(QPTR(p), pool->alloc_size,
					    LIBNUMA_ONLY_ARG(mypool->node)
					    pool->alignment);
		j++;
		p = mypool->alloc_list[j];
	    }
	    p = mypool->alloc_list;
	    mypool->alloc_list =
		mypool->alloc_list[pagesize / sizeof(void *) - 1];
	    free(p);
	}
    }
    VALGRIND_DESTROY_MEMPOOL(pool);
    free(pool->pools);
    free(pool);
}				       /*}}} */
