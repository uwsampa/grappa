/* #include <sys/types.h> */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "globals.h"

void getStartLists(graph* GPtr, int *numMarkedEdges, int *markedEdges)
{ int i, NE, maxWeight, *weight;

  NE     = GPtr->numEdges;
  weight = GPtr->intWeight;

/* Find maximum integer weight */
  maxWeight = 0;
  for (i = 0; i < NE; i++){
    maxWeight = (weight[i] > maxWeight) ? weight[i] : maxWeight;
  }

/* Store maximum weight edge labels */
  *numMarkedEdges = 0;

  #pragma mta assert no dependence
  #pragma mta block dynamic schedule
  #pragma mta use 100 streams
  for (i = 0; i < NE; i++) {
      if (weight[i] == maxWeight) {
         int k = int_fetch_add(numMarkedEdges, 1);
         markedEdges[k] = i;
  }   }

  printf("%9d edges with maximum weight %d\n", *numMarkedEdges, maxWeight);
}
