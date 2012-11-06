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

#ifndef SHARED_DLIST_H
#define SHARED_DLIST_H

#include <upc_relaxed.h>

typedef shared struct shr_dcell * shr_dlist;

struct shr_dcell
{
  shared void *element;
  shr_dlist next;
  shr_dlist prev;
};

extern shr_dlist shr_dcons(shared void *element, shr_dlist prev, shr_dlist next);
extern shr_dlist shr_create_and_link(shared void *element, shr_dlist prev, shr_dlist next);
extern shared void* shr_unlink_and_free(shr_dlist l);

#endif /* SHARED_DLIST_H */
