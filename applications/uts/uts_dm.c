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
#include <alloca.h>

#ifdef USING_GTC
#include "tc.h"
#endif

#include "uts.h"
#include "uts_dm.h"

// Define DEBUG_PROGRESS > 0 to get progress messages
// #define DEBUG_PROGRESS 5000000
#ifndef DEBUG_PROGRESS
#define DEBUG_PROGRESS 0
#endif

// parallel execution parameters 
int chunkSize = 20;    // number of nodes to move to/from shared area
int polling_interval = 0;

#ifdef USING_GTC
extern task_class_t uts_tclass;
#endif

// forward declarations
void releaseNodes(StealStack *ss);

/***********************************************************
 *  UTS Implementation Hooks                               *
 ***********************************************************/

// Return a string describing this implementation
char * impl_getName() {
  return ss_get_par_description();
}


// Construct string with this implementation's parameter settings 
int impl_paramsToStr(char *strBuf, int ind) {
  // search method
  ind += sprintf(strBuf+ind, "Execution strategy:  ");
  ind += sprintf(strBuf+ind, "Parallel search using %d threads\n", ss_get_num_threads());
  ind += sprintf(strBuf+ind, "   Load balance by work stealing, chunk size = %d nodes\n",chunkSize);
  ind += sprintf(strBuf+ind, "   Polling Interval: %d\n", polling_interval);
      
  return ind;
}

// Parse command-line flags
int impl_parseParam(char *param, char *value) {
  int err = 0;  // Return 0 on a match, nonzero on an error

  switch (param[1]) {
    case 'c':
      chunkSize = atoi(value); break;
    case 'i':
      polling_interval = atoi(value); break;
    default:
      err = 1;
      break;
  }

  return err;
}

// Add this to the generic help message
void impl_helpMessage() {
  printf("   -c  int   chunksize for work sharing and work stealing\n");
  printf("   -i  int   work stealing/sharing interval (stealing default: adaptive)\n");
}


void impl_abort(int err) {
  ss_abort(err);
}


/***********************************************************
 *  UTS Implementation                                     *
 ***********************************************************/

/* Fatal error */
void ss_error(char *str, int error)
{
	fprintf(stderr, "*** [Thread %i] %s\n", ss_get_thread_num(), str);
	ss_abort(error);
}


/* 
 * Generate all children of the parent
 *
 * details depend on tree type, node type and shape function
 *
 */
void genChildren(Node * parent, void * child_buf, Node * child, StealStack * ss) {
  int parentHeight = parent->height;
  int numChildren, childType;

  ss->maxTreeDepth = max(ss->maxTreeDepth, parent->height);

  numChildren = uts_numChildren(parent);
  childType   = uts_childType(parent);

  // record number of children in parent
  parent->numChildren = numChildren;

  // construct children and push onto stack
  if (numChildren > 0) {
    int i, j;
    child->type = childType;
    child->height = parentHeight + 1;

    for (i = 0; i < numChildren; i++) {
      for (j = 0; j < computeGranularity; j++) {
        // TBD:  add parent height to spawn
        // computeGranularity controls number of rng_spawn calls per node
        rng_spawn(parent->state.state, child->state.state, i);
      }

      ss_put_work(ss, child_buf);
    }
  } else {
    ss->nLeaves++;
  }
}


/* 
 * parallel search of UTS trees using work stealing 
 * 
 *   Note: tree size is measured by the number of
 *         push operations
 */

#ifdef USING_GTC
#include "tc.h"
#endif

void parTreeSearch(StealStack *ss) {
  Node *parent;
  Node *child;
  void *parent_buf, *child_buf;

#ifdef USING_GTC
  parent_buf = (void*) gtc_task_create_ofclass(sizeof(Node), uts_tclass);
  parent     = gtc_task_body((task_t*)parent_buf);
  child_buf  = (void*) gtc_task_create_ofclass(sizeof(Node), uts_tclass);
  child      = gtc_task_body((task_t*)child_buf);
#else
  child      = malloc(sizeof(Node));
  parent     = malloc(sizeof(Node));
  parent_buf = parent;
  child_buf  = child;
#endif

  while (ss_get_work(ss, parent_buf) == STATUS_HAVEWORK) {
      genChildren(parent, child_buf, child, ss);
#if DEBUG_PROGRESS > 0
      // Debugging: Witness progress...
      if (ss->nNodes % DEBUG_PROGRESS == 0)
      	printf("Thread %3d: Progress is %d nodes\n", ss_get_thread_num(), ss->nNodes);
#endif
  }

#ifdef USING_GTC
  gtc_task_destroy(parent_buf);
  gtc_task_destroy(child_buf);
#else
  free(parent);
  free(child);
#endif
}


void showStats() {
  int i, j;
  counter_t tnodes = 0, tleaves = 0, trel = 0, tacq = 0, tsteal = 0, tfail= 0;
  counter_t mdepth = 0, mheight = 0;
  double twork = 0.0, tsearch = 0.0, tidle = 0.0, tovh = 0.0;
  double max_times[SS_NSTATES];
  double min_times[SS_NSTATES];
  double elapsedSecs;
  int num_workers;
  StealStack *stealStack;

  stealStack = malloc(sizeof(StealStack)*ss_get_num_threads());
  if (!stealStack)
    ss_error("showStats(): out of memory\n", 10);

  /* Gather the stats and return if I'm not the one that has them */
  if (!ss_gather_stats(stealStack, &num_workers))
    return;

  for (i = 0; i < SS_NSTATES; i++) {
    max_times[i] = 0.0;
    min_times[i] = stealStack[0].time[i];
  }

  elapsedSecs = stealStack[0].walltime;

  // combine measurements from all threads
  for (i = 0; i < num_workers; i++) {
    tnodes  += stealStack[i].nNodes;
    tleaves += stealStack[i].nLeaves;
    trel    += stealStack[i].nRelease;
    tacq    += stealStack[i].nAcquire;
    tsteal  += stealStack[i].nSteal;
    tfail   += stealStack[i].nFail;
    twork   += stealStack[i].time[SS_WORK];
    tsearch += stealStack[i].time[SS_SEARCH];
    tidle   += stealStack[i].time[SS_IDLE];
    tovh    += stealStack[i].time[SS_OVH];
    mdepth   = max(mdepth, stealStack[i].maxStackDepth);
    mheight  = max(mheight, stealStack[i].maxTreeDepth);

    for (j = 0; j < SS_NSTATES; j++) {
      if (max_times[j] < stealStack[i].time[j])
        max_times[j] = stealStack[i].time[j];
      if (min_times[j] > stealStack[i].time[j])
        min_times[j] = stealStack[i].time[j];
    }
  }
  if (trel != tacq + tsteal) {
    printf("*** error! total released != total acquired + total stolen\n");
  }

  uts_showStats(ss_get_num_threads(), chunkSize, elapsedSecs, tnodes, tleaves, mheight);

  if (verbose > 1) {
    printf("Total chunks released = %d, of which %d reacquired and %d stolen\n",
           trel, tacq, tsteal);
    printf("Failed steals = %d, Max queue size = %d\n", tfail, mdepth);
    printf("Avg time per thread: Work = %.6f, Overhead = %6f, Search = %.6f, Idle = %.6f.\n", (twork / ss_get_num_threads()),
           (tovh / ss_get_num_threads()), (tsearch / ss_get_num_threads()), (tidle / ss_get_num_threads()));
    printf("Min time per thread: Work = %.6f, Overhead = %6f, Search = %.6f, Idle = %.6f.\n", min_times[SS_WORK], min_times[SS_OVH],
           min_times[SS_SEARCH], min_times[SS_IDLE]);
    printf("Max time per thread: Work = %.6f, Overhead = %6f, Search = %.6f, Idle = %.6f.\n\n", max_times[SS_WORK], max_times[SS_OVH],
           max_times[SS_SEARCH], max_times[SS_IDLE]);
  }

  // per thread execution info
  if (verbose > 2) {
    for (i = 0; i < num_workers; i++) {
      printf("** Thread %d\n", i);
      printf("  # nodes explored    = %d\n", stealStack[i].nNodes);
      printf("  # chunks released   = %d\n", stealStack[i].nRelease);
      printf("  # chunks reacquired = %d\n", stealStack[i].nAcquire);
      printf("  # chunks stolen     = %d\n", stealStack[i].nSteal);
      printf("  # failed steals     = %d\n", stealStack[i].nFail);
      printf("  maximum stack depth = %d\n", stealStack[i].maxStackDepth);
      printf("  work time           = %.6f secs (%d sessions)\n",
             stealStack[i].time[SS_WORK], stealStack[i].entries[SS_WORK]);
      printf("  overhead time       = %.6f secs (%d sessions)\n",
             stealStack[i].time[SS_OVH], stealStack[i].entries[SS_OVH]);
      printf("  search time         = %.6f secs (%d sessions)\n",
             stealStack[i].time[SS_SEARCH], stealStack[i].entries[SS_SEARCH]);
      printf("  idle time           = %.6f secs (%d sessions)\n",
             stealStack[i].time[SS_IDLE], stealStack[i].entries[SS_IDLE]);
      printf("\n");
    }
  }

#ifdef TRACE
  ss_printTrace(stealStack, num_workers);
#endif
}


int main(int argc, char *argv[]) {
  double t1, t2;
  void *root_buf;
  Node *root;
  StealStack *ss;

  /* initialize stealstacks and comm. layer */
  ss = ss_init(&argc, &argv);

  /* determine benchmark parameters */
  uts_parseParams(argc, argv);

  /* Initialize trace collection structures */
  ss_initStats(ss);
 
  /* show parameter settings */
  if (ss_get_thread_num() == 0) {
      uts_printParams();
  }
  
#ifdef USING_GTC
  root_buf = gtc_task_create_ofclass(sizeof(Node), uts_tclass);
  root     = gtc_task_body(root_buf);
#else
  root_buf = alloca(sizeof(Node));
  root     = root_buf;
#endif

  fflush(NULL);

  // Workers will return 1 from ss_start(), all others (managers)
  // will return 0 here once the computation ends
  if (ss_start(sizeof(Node), chunkSize)) {

      /* initialize root node and push on thread 0 stack */
      if (ss_get_thread_num() == 0) {
          uts_initRoot(root, type);
#ifdef TRACE
	  ss_markSteal(ss, 0); // first session is own "parent session"
#endif
          ss_put_work(ss, root_buf);
      }
  
      /* time parallel search */
      t1 = uts_wctime();
      parTreeSearch(ss);
      t2 = uts_wctime();
      ss->walltime = t2 - t1;
#ifdef TRACE
      ss->startTime = t1;
      ss->sessionRecords[SS_IDLE][ss->entries[SS_IDLE] - 1].endTime = t2;
#endif
  }

  ss_stop();

  /* display results */
  showStats();

#ifdef USING_GTC
  gtc_task_destroy(root_buf);
#endif

  ss_finalize();

  return 0;
}
