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

#ifndef DLIST_H
#define DLIST_H

typedef struct dcell *dlist;

struct dcell
{
  void *element;
  dlist next;
  dlist prev;
};

extern dlist dcons(void *element, dlist prev, dlist next);
extern dlist create_and_link(void *element, dlist prev, dlist next);
extern void* unlink_and_free(dlist l);

#endif
