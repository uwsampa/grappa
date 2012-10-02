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
#include "shared_dlist.h"

shr_dlist shr_dcons(shared void *element, shr_dlist prev, shr_dlist next) {
	shr_dlist temp = upc_alloc(sizeof(struct shr_dcell));

	temp -> element = element;
	temp -> prev = prev;
	temp -> next = next;

	return temp;
}

shr_dlist shr_create_and_link(shared void *element, shr_dlist prev, shr_dlist next) {
	shr_dlist temp = shr_dcons(element, prev, next);

	prev -> next = temp;
	next -> prev = temp;

	return temp;
}

shared void* shr_unlink_and_free(shr_dlist l) {
	shared void *temp;

	if (l == NULL) return NULL;
	temp = l -> element;
	//printf("unlink_and_free(0x%0x-%d): Called by PE %d, work=0x%x-%d\n", upc_addrfield(l), upc_threadof(l), MYTHREAD,
	//	upc_addrfield(temp), upc_threadof(temp));
	//fflush(NULL);
	l -> next -> prev = l -> prev;
	l -> prev -> next = l -> next;
	upc_free(l);

	return temp;
}  
