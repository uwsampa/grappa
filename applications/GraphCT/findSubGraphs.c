#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif
#include "defs.h"
#include "globals.h"

/* Recursive BFW algorithm */
void markSubGraph(int64_t L, int64_t head, int64_t *eV, int64_t *start, int64_t *sgv) {
  int64_t i;
  if (L == subGraphPathLength - 1) return;     /* don't mark leaf nodes */

MTA("mta assert parallel")
MTA("mta loop future")
  for (i = start[head]; i < start[head + 1]; i++) {
      int64_t tail  = eV[i];
      int64_t level = readfe(sgv + tail);

/* Node visited at an earlier or the same level */
      if (L >= level) {
         writeef(sgv + tail, level);

/* Node not visited or visited at a latter level */
      } else {
         writeef(sgv + tail, L + 1);
         markSubGraph(L + 1, tail, eV, start, sgv);
} }   }


void findSubGraphs(int64_t nSG, int64_t *markedEdges, graph *G)
{ int64_t i, j, NV, nodes, edges;
  int64_t *sV, *eV, *start, *sgv;

  NV     = G->numVertices;
  sV     = G->startVertex;
  eV     = G->endVertex;
  start  = G->edgeStart;

  sgv = (int64_t *) xmalloc(NV * sizeof(int64_t));

/* For each edge (head -> tail) marked in Kernel 2, find all the nodes
   a distance subGraphPathLength away from head.
*/
  for (i = 0; i < nSG; i++) {
      int64_t head = sV[markedEdges[i]];
      for (j = 0; j < NV; j++) sgv[j] = subGraphPathLength;

/* Mark nodes in subgraph */
      sgv[head] = 0;
      markSubGraph(0, head, eV, start, sgv);

/* Count nodes and edges in subgraph */
      nodes = edges = 0;
      for (j = 0; j < NV; j++)
          if (sgv[j] < subGraphPathLength)
             {nodes ++; edges += start[j + 1] - start[j];}

      printf("Subgraph %2ld with %8ld nodes and %9ld edges\n", i, nodes, edges);
  }

  free(sgv);
}
