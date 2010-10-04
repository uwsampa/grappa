#ifndef _STCONNECTIVITY_H
#define _STCONNECTIVITY_H

#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "graph.h"
#include "defs.h"

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#define WHITE 0
#define RED 1
#define GREEN 33554432

double STConnectivity(graph *G, int s, int t);

#endif
