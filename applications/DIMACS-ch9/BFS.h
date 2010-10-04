#ifndef _BFS_H
#define _BFS_H

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

#define WHITE 0
#define BLACK 1

double BFS(graph *G, int s);

#endif
