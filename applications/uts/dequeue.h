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

#ifndef DEQUEUE_H
#define DEQUEUE_H
#include "dlist.h"

struct dequeue{
  dlist head;
  dlist tail;
};
typedef struct dequeue *dequeue;

/* create an empty dqueue */
extern dequeue deq_create();

/* insert an element at the front of the dqueue */
extern void deq_pushFront(dequeue q, void *element);

/* insert an element at the back of the dqueue */ 
extern void deq_pushBack(dequeue q, void *element);

/* delete an element from the front of the dqueue and return it */
extern void *deq_popFront(dequeue q);

/* delete an element from the back of the dqueue and return it */
extern void *deq_popBack(dequeue q);

/* return a true value if and only if the dqueue is empty */
extern int deq_isEmpty(dequeue q);

/*get the front without removing it from the dequeue*/
extern void* deq_peekFront(dequeue q);

/*get teh back without removing it from the dequeue*/
extern void* deq_peekBack(dequeue q);

extern void deq_mkEmpty(dequeue q);
extern int deq_length(dequeue q);
#endif
