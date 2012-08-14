/* Modified UTS benchmark for purposes of making an apples to apples comparison between the XMT
** and Grappa on tree traversal providing the advantage to the XMT of timing only the tree traversal
** not the random number generation by precomputing the tree; and also to replacing that random
** number generation with a parameterized computation that accesses remote memory and pressures
** the cache.  The larger the parameter, the larger the array stored at each node;  the computation
** merely adds the parent's array to the contents of the child's.  This both increases the remote
** access (good for XMT) and puts pressure on x86 cache.
*/
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "uts.h"

#ifdef __MTA__
/***********************************************************
 *     Cray MTA 2 -- Multithreaded Execution               *
 ***********************************************************/
/* don't want any loops parallelized: */
#pragma mta parallel off

#include <sys/mta_task.h>
#include <machine/mtaops.h>
#include <machine/runtime.h>

#define PARALLEL         1
#define COMPILER_TYPE    4
#define SHARED
#define SHARED_INDEF
#define VOLATILE
#define MAXTHREADS 1
#define LOCK_T           void
#define GET_NUM_THREADS  mta_get_num_teams()
#define GET_THREAD_NUM   0
#define SET_LOCK(zlk)    
#define UNSET_LOCK(zlk)  
#define INIT_LOCK(zlk) 
#define INIT_SINGLE_LOCK(zlk) 
#define SMEMCPY          memcpy
#define ALLOC            malloc
#define BARRIER           

#else
/***********************************************************
 *     (default) ANSI C compiler - sequential execution    *
 ***********************************************************/
#define PARALLEL         0
#define COMPILER_TYPE    0
#define SHARED
#define SHARED_INDEF
#define VOLATILE
#define MAXTHREADS 1
#define LOCK_T           void
#define GET_NUM_THREADS  1
#define GET_THREAD_NUM   0
#define SET_LOCK(zlk)    
#define UNSET_LOCK(zlk)  
#define INIT_LOCK(zlk) 
#define INIT_SINGLE_LOCK(zlk) 
#define SMEMCPY          memcpy
#define ALLOC            malloc
#define BARRIER           
#endif


/***********************************************************
 *  Global state                                           *
 ***********************************************************/
counter_t nNodes  = 0;
counter_t nLeaves = 0;
counter_t maxTreeDepth = 0;

counter_t global_id = 0;
counter_t global_child_index = 0;
int NumChildren[1<<31];
int ChildIndex[1<<31];
int Child[1<<31];
int Payload[1<<31];

/***********************************************************
 *  UTS Implementation Hooks                               *
 ***********************************************************/

// The name of this implementation
char * impl_getName() {
#ifdef __MTA__
  return "MTA Futures-Parallel Recursive Search";
#else
  return "Sequential Recursive Search";
#endif
}

int  impl_paramsToStr(char *strBuf, int ind) { 
  ind += sprintf(strBuf+ind, "Execution strategy:  %s\n", impl_getName());
#ifdef __MTA__
  ind += sprintf(strBuf+ind, "  MTA parallel search using %d teams, %d max teams\n", mta_get_num_teams(), mta_get_max_teams()); 
#endif
  return ind;
}

// Not using UTS command line params, return non-success
int  impl_parseParam(char *param, char *value) { return 1; }

void impl_helpMessage() {
  printf("   none.\n");
}

void impl_abort(int err) {
  exit(err);
}


/***********************************************************
 * Recursive depth-first implementation                    *
 ***********************************************************/

typedef struct {
  counter_t maxdepth, size, leaves;
} Result;

#ifdef __MTA__

int threshold = 4;
void parallel_loop(int start_index, int iterations, void (*loop_body)(int i, void *sargs), void * shared_args) {
//  switch (iterations) {
//  case 0:
//    return;
//  case 1:
//    loop_body(start_index, shared_args);
//    break;
//  default:
  if (iterations < threshold) {
    for (int iter=0; iter<iterations; iter++) {
      loop_body(start_index+iter, shared_args);
    }
  } else {
    {
      future void d$;
      future d$ (start_index, iterations, loop_body, shared_args) {
	parallel_loop(start_index + (iterations+1)/2, iterations/2, loop_body, shared_args);
      }
      parallel_loop (start_index, (iterations+1)/2, loop_body, shared_args);
      touch(&d$);
    }
  }
}


Result parTreeSearch(int depth, int id);

typedef struct {
  int childType;
  counter_t parentHeight;
  Node * parent;
  int depth;
  Result * r;
  int childid0;
} sibling_args;

typedef struct {
  int parentid;
  int depth;
  Result * r;
  int childid0;
} sibling_args_search;

void explore_child (int i, void* sv) {
  sibling_args_search *s = (sibling_args_search*) sv;
  int id = s->childid0 + i;
  int pid = s->parentid;
  for (i = 0; i < payloadSize; i++) Payload[(id*payloadSize+i)&0x3fffffff]+=Payload[(pid*payloadSize+i)&0x3fffffff];
  Result c = parTreeSearch(s->depth+1, id);
  if (c.maxdepth>s->r->maxdepth) {
    int d = readfe(&(s->r->maxdepth));
    if (c.maxdepth>d) d = c.maxdepth;
    s->r->maxdepth = d;
  }
  int_fetch_add(&(s->r->size), c.size); // Note: on MTA sizeof(int)=8 and sizeof(counter_t)=8
  int_fetch_add(&(s->r->leaves), c.leaves);
}
#endif /* __MTA__ */

Result parTreeSearch(int depth, int id) {

  Result r = { depth, 1, 0 };

  int numChildren = NumChildren[id];
  int childid0 = Child[ChildIndex[id]];

  // Recurse on the children
  if (numChildren > 0) {
    sibling_args_search args = {id, depth, &r, childid0}; 
    parallel_loop(0, numChildren, explore_child, &args);
  } else {
    r.leaves = 1;
  }

  return r;
}

Result parTreeCreate(int depth, Node *parent);
void create_children (int i, void* sv) {
  sibling_args *s = (sibling_args*) sv;
  int j;
  Node child;
  child.type = s->childType;
  child.height = s->parentHeight + 1;
  child.numChildren = -1;    // not yet determined
  child.id = s->childid0 + i;
  for (j = 0; j < computeGranularity; j++) {
    rng_spawn(s->parent->state.state, child.state.state, i);
  }
  Result c = parTreeCreate(s->depth+1, &child);
  if (c.maxdepth>s->r->maxdepth) {
    int d = readfe(&(s->r->maxdepth));
    if (c.maxdepth>d) d = c.maxdepth;
    s->r->maxdepth = d;
  }
  int_fetch_add(&(s->r->size), c.size); // Note: on MTA sizeof(int)=8 and sizeof(counter_t)=8
  int_fetch_add(&(s->r->leaves), c.leaves);
}

Result parTreeCreate(int depth, Node *parent) {
  int numChildren, childType;
  counter_t parentHeight = parent->height;
  counter_t subtreesize;

  Result r = { depth, 1, 0 };

  numChildren = uts_numChildren(parent);
  childType   = uts_childType(parent);

  /*** Added for the sake of remembering the tree created: ***/
  int id = uts_nodeId(parent);
  /* Record the number of children: */
  NumChildren[id] = numChildren;
  /* Assign fresh unique ids for the children: */
  int childid0 = int_fetch_add(&global_id, numChildren);
  /* Record ids and indices of the children: */
  int index = ChildIndex[id] = int_fetch_add(&global_child_index, numChildren);
  for (int i = 0; i < numChildren; i++) {
    Child[index+i] = childid0 + i;
  }
  /***********************************************************/

  // record number of children in parent
  parent->numChildren = numChildren;
  subtreesize = numChildren;
  
  // Recurse on the children
  if (numChildren > 0) {
    int i, j;
#ifdef __MTA__
    sibling_args args = {childType, parentHeight, parent, depth, &r, childid0};
    parallel_loop(0, numChildren, create_children, &args);
#else
    for (i = 0; i < numChildren; i++) {
      Node child;
      child.type = childType;
      child.height = parentHeight + 1;
      child.numChildren = -1;    // not yet determined
      for (j = 0; j < computeGranularity; j++) {

      }
      Result c = parTreeSearch(depth+1, &child);

      if (c.maxdepth>r.maxdepth) r.maxdepth = c.maxdepth;
      r.size += c.size;
      r.leaves += c.leaves;
    }
#endif /* __MTA__ */
  } else {
    r.leaves = 1;
  }

  return r;
}

int main(int argc, char *argv[]) {
  Node root;
  double t1=0.0, t2=0.0;

  fprintf(stderr, "%lu %lu %lu", Child, NumChildren, ChildIndex);
  
  uts_parseParams(argc, argv);

  if (GET_THREAD_NUM == 0) {
    uts_printParams();
    uts_initRoot(&root, type);
  }

  fprintf(stderr, "creating tree....\n");
  global_id = 1;
  root.id = 0;
  
  t1 = uts_wctime();
  Result r = parTreeCreate(0, &root);
  t2 = uts_wctime();

  maxTreeDepth = r.maxdepth;
  nNodes  = r.size;
  nLeaves = r.leaves;

  if (GET_THREAD_NUM == 0) {
    uts_showStats(GET_NUM_THREADS, 0, t2-t1, nNodes, nLeaves, maxTreeDepth);
  } 

  fprintf(stderr, "searching tree once....\n");
#pragma mta fence
  t1 = uts_wctime();

  r = parTreeSearch(0, 0);

#pragma mta fence
  t2 = uts_wctime();
  
  maxTreeDepth = r.maxdepth;
  nNodes  = r.size;
  nLeaves = r.leaves;

  if (GET_THREAD_NUM == 0) {
    uts_showStats(GET_NUM_THREADS, 0, t2-t1, nNodes, nLeaves, maxTreeDepth);
  } 
  
  
  fprintf(stderr, "searching tree twice....\n");
#pragma mta fence
  t1 = uts_wctime();

  r = parTreeSearch(0, 0);

#pragma mta fence
  t2 = uts_wctime();

  maxTreeDepth = r.maxdepth;
  nNodes  = r.size;
  nLeaves = r.leaves;

  if (GET_THREAD_NUM == 0) {
    uts_showStats(GET_NUM_THREADS, 0, t2-t1, nNodes, nLeaves, maxTreeDepth);
  } 
  
  return 0;
}
