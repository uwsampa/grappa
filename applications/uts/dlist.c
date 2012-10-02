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

#include "dlist.h"
#include <stdlib.h>

dlist dcons(void *element, dlist prev, dlist next){
  dlist temp = malloc(sizeof(struct dcell));
  temp -> element = element;
  temp -> prev = prev;
  temp -> next = next;
  return temp;
}

dlist create_and_link(void *element, dlist prev, dlist next){
  dlist temp = dcons(element, prev, next);
  prev -> next = temp;
  next -> prev = temp;
  return temp;
}

void* unlink_and_free(dlist l){
  void *temp = l -> element;
  l -> next -> prev = l -> prev;
  l -> prev -> next = l -> next;
  free(l);
  return temp;
}  
