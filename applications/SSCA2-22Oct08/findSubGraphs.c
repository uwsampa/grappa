#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "globals.h"

/* Recursive BFW algorithm */
void markSubGraph(int L, int head, int *eV, int *start, int *sgv) {
  int i;
  if (L == subGraphPathLength - 1) return;     /* don't mark leaf nodes */

#pragma mta assert parallel
#pragma mta loop future
  for (i = start[head]; i < start[head + 1]; i++) {
      int tail  = eV[i];
      int level = readfe(sgv + tail);

/* Node visited at an earlier or the same level */
      if (L >= level) {
         writeef(sgv + tail, level);

/* Node not visited or visited at a latter level */
      } else {
         writeef(sgv + tail, L + 1);
         markSubGraph(L + 1, tail, eV, start, sgv);
} }   }


void findSubGraphs(int nSG, int *markedEdges, graph *G)
{ int i, j, NV, nodes, edges;
  int *sV, *eV, *start, *sgv;

  NV     = G->numVertices;
  sV     = G->startVertex;
  eV     = G->endVertex;
  start  = G->edgeStart;

  sgv = (int *) malloc(NV * sizeof(int));

/* For each edge (head -> tail) marked in Kernel 2, find all the nodes
   a distance subGraphPathLength away from head.
*/
  for (i = 0; i < nSG; i++) {
      int head = sV[markedEdges[i]];
      for (j = 0; j < NV; j++) sgv[j] = subGraphPathLength;

/* Mark nodes in subgraph */
      sgv[head] = 0;
      markSubGraph(0, head, eV, start, sgv);

/* Count nodes and edges in subgraph */
      nodes = edges = 0;
      for (j = 0; j < NV; j++)
          if (sgv[j] < subGraphPathLength)
             {nodes ++; edges += start[j + 1] - start[j];}

      printf("Subgraph %2d with %8d nodes and %9d edges\n", i, nodes, edges);
  }

  free(sgv);
}
