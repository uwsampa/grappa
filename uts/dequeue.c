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

#include "dequeue.h"
#include "dlist.h"
#include <stdlib.h>

dequeue deq_create(){
  dequeue q = malloc(sizeof(struct dequeue));
  q -> head = dcons(NULL, NULL, NULL);
  q -> tail = dcons(NULL, NULL, NULL);
  q -> head -> next = q -> tail;
  q -> tail -> prev = q -> head;
  return q;
}

int deq_isEmpty(dequeue q) {
  return q -> head -> next == q -> tail;
}

void deq_pushFront(dequeue q, void *element) {
  create_and_link(element, q->head, q->head->next);
}

void deq_pushBack(dequeue q, void *element){
  create_and_link(element, q->tail->prev, q->tail);
}

void* deq_popFront(dequeue q){
  return unlink_and_free(q->head->next);
}

void* deq_popBack(dequeue q) {
  return unlink_and_free(q->tail->prev);
}

void* deq_peekFront(dequeue q){
  return q->head->next->element;
}

void* deq_peekBack(dequeue q){
  return q->tail->prev->element;
}

void deq_mkEmpty(dequeue q){
  while(!deq_isEmpty(q))
    deq_popFront(q);
}

int deq_length(dequeue q) {
	int count;
	dlist e = q->head->next;

	for (count = 0; e != q->tail; count++)
	e = e->next;

	return count;
}
