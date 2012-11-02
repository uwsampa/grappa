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

#include <stdio.h>
#include "upc_strict.h"
#include "shared_dlist.h"
#include "shared_dequeue.h"

shr_dequeue shr_deq_create() {
	shr_dequeue q = upc_alloc(sizeof(struct shr_dequeue_t));
	q -> head = shr_dcons(NULL, NULL, NULL);
	q -> tail = shr_dcons(NULL, NULL, NULL);
	q -> head -> next = q -> tail;
	q -> tail -> prev = q -> head;
	//printf("shr_deq_create(): PE %d, Queue at 0x%0x-%d, head=0x%0x-%d, tail=%0x-%d\n", MYTHREAD, upc_addrfield(q), upc_threadof(q),
	//	upc_addrfield(q->head), upc_threadof(q->head), upc_addrfield(q->tail), upc_threadof(q->tail));
	return q;
}

int shr_deq_isEmpty(shr_dequeue q) {
	return q -> head -> next == q -> tail;
}

void shr_deq_pushFront(shr_dequeue q, shared void *element) {
//	printf("shr_deq_pushFront(0x%x-%d): pushing 0x%0x-%d\n", upc_addrfield(q), upc_threadof(q),
//		upc_addrfield(element), upc_threadof(element));
	shr_create_and_link(element, q->head, q->head->next);
}

void shr_deq_pushBack(shr_dequeue q, shared void *element){
//	printf("shr_deq_pushBack(0x%x-%d): pushing 0x%0x-%d\n", upc_addrfield(q), upc_threadof(q),
//		upc_addrfield(element), upc_threadof(element));
	shr_create_and_link(element, q->tail->prev, q->tail);
}

shared void* shr_deq_popFront(shr_dequeue q){
//	printf("shr_deq_popFront(0x%x-%d): popping 0x%0x-%d\n", upc_addrfield(q), upc_threadof(q),
//		upc_addrfield(q->head->next), upc_threadof(q->head->next));
	if (shr_deq_isEmpty(q)) return NULL;
	return shr_unlink_and_free(q->head->next);
}

shared void* shr_deq_popBack(shr_dequeue q) {
//	printf("shr_deq_popBack(0x%x-%d): popping 0x%0x-%d\n", upc_addrfield(q), upc_threadof(q),
//		upc_addrfield(q->tail->prev), upc_threadof(q->tail->prev));
	if (shr_deq_isEmpty(q)) return NULL;
	return shr_unlink_and_free(q->tail->prev);
}

shared void* shr_deq_peekFront(shr_dequeue q){
	return q->head->next->element;
}

shared void* shr_deq_peekBack(shr_dequeue q){
	return q->tail->prev->element;
}

void shr_deq_mkEmpty(shr_dequeue q){
	while(!shr_deq_isEmpty(q))
		shr_deq_popFront(q);
}

int shr_deq_length(shr_dequeue q) {
	int count;
	shr_dlist e = q->head->next;

	for (count = 0; e != q->tail; count++)
	e = e->next;

	return count;
}
