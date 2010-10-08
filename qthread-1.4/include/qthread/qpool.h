#ifndef QPOOL_H
#define QPOOL_H

#include <stddef.h>		       /* for size_t (according to C89) */

#include <qthread/qthread.h>

Q_STARTCXX /* */

typedef struct qpool_s qpool;

void *qpool_alloc(qthread_t * me, qpool * pool);

void qpool_free(qthread_t * me, qpool * pool, void *mem);

qpool *qpool_create(const size_t item_size);
qpool *qpool_create_aligned(const size_t item_size, const size_t alignment);

void qpool_destroy(qpool * pool);

Q_ENDCXX /* */

#endif
