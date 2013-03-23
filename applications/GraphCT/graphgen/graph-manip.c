#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include "defs.h"

void
free_graph(graph * G)
{
  if(G != NULL)
    munmap (G->startVertex, G->map_size);
}

void
alloc_graph (graph * G, int64_t NV, int64_t NE)
{
  if (!G) return;

  G->numEdges    = NE;
  G->numVertices = NV;
  G->map_size = (3 * NE + 2 * NV + 2) * sizeof (int64_t);

  G->startVertex = (int64_t *)xmmap (NULL, G->map_size, PROT_READ | PROT_WRITE,
				 MAP_PRIVATE | MAP_ANON, 0,0);
  G->endVertex   = &G->startVertex[NE];
  G->edgeStart   = &G->endVertex[NE];
  G->intWeight   = &G->edgeStart[NV+2];
  G->marks       = &G->intWeight[NE];
}

void
alloc_edgelist (graphSDG * G, int64_t NE)
{
  if (!G) return;

  G->numEdges    = NE;
  G->map_size = (3 * NE) * sizeof (int64_t);
  G->startVertex = (int64_t *)xmmap (NULL, G->map_size, PROT_READ | PROT_WRITE,
				 MAP_PRIVATE | MAP_ANON, 0,0);
  G->endVertex   = &G->startVertex[NE];
  G->intWeight   = &G->endVertex[NE];
}

void
free_edgelist (graphSDG * G)
{
  if (G)
    munmap (G->startVertex, G->map_size);
}
