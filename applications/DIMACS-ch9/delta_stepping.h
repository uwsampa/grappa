#ifndef _DELTA__STEPPING_H
#define _DELTA__STEPPING_H

#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "graph.h"
#include "defs.h"

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include <sys/time.h>
#endif

#define INIT_SIZE 100
#define FREEMEM 1
#define SORTW 1

extern double delta;
extern int numBuckets;
extern double INFTY;

double delta_stepping(graph*, int, double*);
int relax(int*, double*, int*, int, graph*, double*, int**, int, int, int*, int*, int*, int*, int*, int*);
#endif
