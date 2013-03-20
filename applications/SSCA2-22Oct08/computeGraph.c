#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "defs.h"
#include "globals.h"

/* A bucket sort */
void SortStart(NV, NE, sv1, ev1, sv2, ev2, start)
  int NV, NE, *sv1, *ev1, *sv2, *ev2, *start;
{ int i;
  for (i = 0; i < NV + 2; i++) start[i] = 0;

  start += 2;

/* Histogram key values */
#pragma mta assert no alias *start *sv1
  for (i = 0; i < NE; i++) start[sv1[i]] ++;

/* Compute start index of each bucket */
  for (i = 1; i < NV; i++) start[i] += start[i-1];

  start --;

/* Move edges into its bucket's segment */
#pragma mta assert no dependence
  for (i = 0; i < NE; i++) {
      int index = start[sv1[i]] ++;
      sv2[index] = sv1[i];
      ev2[index] = ev1[i];
} }


void computeGraph(graph* G, graphSDG* SDGdata) {
  int i, NE, NV;
  int *sv1, *sv2, *ev1, *ev2, *start;

  NE  = SDGdata->numEdges;
  sv1 = SDGdata->startVertex;
  ev1 = SDGdata->endVertex;

  NV = 0;
  for (i = 0; i < NE; i++) NV = (NV > sv1[i]) ? NV : sv1[i];
  for (i = 0; i < NE; i++) NV = (NV > ev1[i]) ? NV : ev1[i];

  NV ++;
  sv2   = (int *) malloc(NE * sizeof(int));
  ev2   = (int *) malloc(NE * sizeof(int));
  start = (int *) malloc((NV + 2) * sizeof(int));

/*-------------------------------------------------------------------------*/
/* STEP 0: Sort edges                                                      */
/*-------------------------------------------------------------------------*/

  SortStart(NV, NE, sv1, ev1, sv2, ev2, start);

  G->numEdges    = NE;
  G->numVertices = NV;
  G->startVertex = sv2;
  G->endVertex   = ev2;
  G->edgeStart   = start;
  G->intWeight   = SDGdata->intWeight;

  free(sv1); free(ev1);
}
