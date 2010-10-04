#ifndef _BELLMAN__FORD_H
#define _BELLMAN__FORD_H

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

double BellmanFord(graph*, int, double*);
#endif
