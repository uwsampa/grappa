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

/***********************************************************
 *                                                         *
 *  TRACING EXTENSION TO UTS:                              *
 *                                                         *
 ***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "uts_dm.h"

#ifndef sgi  
double wctime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + 1E-6 * tv.tv_usec);
}
#else
double wctime() {
  timespec_t tv;
  double time;
  clock_gettime(CLOCK_SGI_CYCLE,&tv);
  time = ((double) tv.tv_sec) + ((double)tv.tv_nsec / 1e9);
  if (debug&16)
    printf("SGI high resolution clock: %f\n",time);
  return time;
}
#endif

/* Initialize trace collection stuff */
void ss_initStats(StealStack *s) {
  int i;
  s->timeLast = wctime();
  for (i = 0; i < SS_NSTATES; i++) {
    s->time[i] = 0.0;
    s->entries[i] = 0;
  }
  s->curState = SS_IDLE;
}

/* Change states */
void ss_setState(StealStack *s, int state) {
  double time;
  if (state < 0 || state >= SS_NSTATES)
    ss_error("ss_setState: thread state out of range", 1);
  if (state == s->curState)
    return;
  time = wctime();
  s->time[s->curState] +=  time - s->timeLast;

#ifdef TRACE
  /* close out last session record */
  s->sessionRecords[s->curState][s->entries[s->curState] - 1].endTime = time;
  if (s->curState == SS_WORK) {
    s->stealRecords[s->entries[SS_WORK] - 1].nodeCount = s->nNodes
      - s->stealRecords[s->entries[SS_WORK] - 1].nodeCount;
  }

  /* initialize new session record */
  s->sessionRecords[state][s->entries[state]].startTime = time;
  if (state == SS_WORK) {
    s->stealRecords[s->entries[SS_WORK]].nodeCount = s->nNodes;
  }
#endif

  s->entries[state]++;
  s->timeLast = time;
  s->curState = state;
}


#ifdef TRACE
// print session records for each thread (used when trace is enabled)
void ss_printTrace(StealStack *s, int numRecords) {
  int i, j, k;
  double offset;

  for (i = 0; i < numRecords; i++) {
    offset = s[i].startTime - s[0].startTime;

    for (j = 0; j < SS_NSTATES; j++)
      for (k = 0; k < s[i].entries[j]; k++) {
        printf ("%d %d %f %f", i, j,
            s[i].sessionRecords[j][k].startTime - offset,
            s[i].sessionRecords[j][k].endTime - offset);
        if (j == SS_WORK)
          printf (" %d %ld",
              s[i].stealRecords[k].victimThread,
              s[i].stealRecords[k].nodeCount);
        printf ("\n");
      }
  }
}

/* Called when we have a successful steal to update our stats */
void ss_markSteal(StealStack *s, int victim) {
  /* update session record of theif */
  s->stealRecords[s->entries[SS_WORK]].victimThread = victim;
}
#endif
