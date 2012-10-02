/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *  
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#ifndef SHARED_DEQUEUE_H
#define SHARED_DEQUEUE_H

#include <upc_relaxed.h>
#include "shared_dlist.h"

struct shr_dequeue_t {
  shr_dlist head;
  shr_dlist tail;
};
typedef shared struct shr_dequeue_t * shr_dequeue;

/* create an empty dqueue */
extern shr_dequeue shr_deq_create();

/* insert an element at the front of the dqueue */
extern void shr_deq_pushFront(shr_dequeue q, shared void *element);

/* insert an element at the back of the dqueue */ 
extern void shr_deq_pushBack(shr_dequeue q, shared void *element);

/* delete an element from the front of the dqueue and return it */
extern shared void *shr_deq_popFront(shr_dequeue q);

/* delete an element from the back of the dqueue and return it */
extern shared void *shr_deq_popBack(shr_dequeue q);

/* return a true value if and only if the dqueue is empty */
extern int shr_deq_isEmpty(shr_dequeue q);

/*get the front without removing it from the dequeue*/
extern shared void* shr_deq_peekFront(shr_dequeue q);

/*get the back without removing it from the dequeue*/
extern shared void* shr_deq_peekBack(shr_dequeue q);

extern void shr_deq_mkEmpty(shr_dequeue q);
extern int shr_deq_length(shr_dequeue q);

#endif /* SHARED_DEQUEUE_H */
