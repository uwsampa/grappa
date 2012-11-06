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
#include <stdlib.h>
#include <malloc.h>

#include "uts.h"

char * impl_getName() {
  return "RNG Timer";
}

void impl_abort(int err) {
  exit(err);
}

// More advanced stuff not used here:
int  impl_paramsToStr(char *strBuf, int ind) { return ind; }
int  impl_parseParam(char *param, char *value) { return 1; }
void impl_helpMessage() {}

//
// Time how long it takes to generate NUM_RNG_SPAWNS children
// to find the average time an rng_spawn() takes.
//
int main(int argc, char *argv[]) {
  Node root, *children;
  double t1, t2, et;
  int i;

#define NUM_RNG_SPAWNS 10000000

#ifdef __MTA__
  printf("MTA -- Starting up with %d teams, %d max\n", mta_get_num_teams(), mta_get_max_teams()); 
#endif

  printf("Timing %d RNG spawns ...\n", NUM_RNG_SPAWNS);

  // Fudge a single node with NUM_RNG_SPAWNS children
  uts_initRoot(&root, 0);
  root.numChildren = NUM_RNG_SPAWNS;
  children = (Node*) malloc(NUM_RNG_SPAWNS * sizeof(Node));

#pragma mta fence
  /* time parallel search */
  t1 = uts_wctime();

#pragma mta assert parallel 
//#pragma mta assert no dependence *children, *parent
//#pragma mta use 100 streams
#pragma mta loop future
  for (i = 0; i < NUM_RNG_SPAWNS; i++) {
    // TBD:  add parent height to spawn
    rng_spawn(root.state.state, children[i].state.state, i);
  }

#pragma mta fence
  t2 = uts_wctime();
  et = t2 - t1;

  /* display results */
  printf("Wall clock time = %.5f sec, %.5e sec/spawn, %.2f spawns/sec\n", 
      et, et/NUM_RNG_SPAWNS, NUM_RNG_SPAWNS/et);

  return 0;
}
