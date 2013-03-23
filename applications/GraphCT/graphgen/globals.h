#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "defs.h"

/* Function definitions */
void getUserParameters();

/* Global Variables */

extern double A;
extern double B;
extern double C;
extern double D;
extern int64_t SCALE;
extern int64_t numVertices;
extern int64_t numEdges;
extern int64_t maxWeight;
extern int64_t K4approx;
extern int64_t subGraphPathLength;

struct stats {
#if defined(__MTA__)
  char statname[257];
  int64_t clock, issues, concurrency, load, store, ifa;
#else
#endif
};

struct stats stats_tic_data, stats_toc_data;

#endif
