/* #include <sys/types.h> */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif
#include "defs.h"
#include "globals.h"

void getStartLists(graph* GPtr, int64_t *numMarkedEdges, int64_t *markedEdges)
{ int64_t i, NE, maxWeight, *weight;

  NE     = GPtr->numEdges;
  weight = GPtr->intWeight;

/* Find maximum integer weight */
  maxWeight = 0;
  for (i = 0; i < NE; i++)
      maxWeight = (weight[i] > maxWeight) ? weight[i] : maxWeight;
  
/* Store maximum weight edge labels */
  *numMarkedEdges = 0;

MTA("mta assert no dependence")
MTA("mta block dynamic schedule")
MTA("mta use 100 streams")
  for (i = 0; i < NE; i++) {
      if (weight[i] == maxWeight) {
         int64_t k = int_fetch_add(numMarkedEdges, 1);
         markedEdges[k] = i;
  }   }

  printf("%9ld edges with maximum weight %ld\n", *numMarkedEdges, maxWeight);
}
