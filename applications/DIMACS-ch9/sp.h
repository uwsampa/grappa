#ifndef _SP_H
#define _SP_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "defs.h"
#include "BFS.h"
#include "STConnectivity.h"
#include "delta_stepping.h"
#include "BellmanFord.h"

double NSSSP(graph*, int, int, int, int);
double P2P(graph*, int, int);
void preprocess_graph(graph*, int*);
#endif
