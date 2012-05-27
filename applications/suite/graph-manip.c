// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>

#include "defs.h"

void free_graph(graph * g) {
  if(g != NULL)
    munmap((caddr_t)g->startVertex, g->map_size);
}

void alloc_graph(graph * g, graphint NV, graphint NE) {
  if (!g) return;
  
  g->numEdges    = NE;
  g->numVertices = NV;
  g->map_size = (3 * NE + 2 * NV + 2) * sizeof(graphint);
  
  g->startVertex = (graphint*)xmmap(NULL, g->map_size, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANON, 0,0);
  g->endVertex   = &g->startVertex[NE];
  g->edgeStart   = &g->endVertex[NE];
  g->intWeight   = &g->edgeStart[NV+2];
  g->marks       = &g->intWeight[NE];
}

void print_graph_dot(graph * g, char* fbase) {
  char fname[512];
  sprintf(fname, "%s%s", fbase, ".graph.dot");
  FILE* f = fopen(fname,"w");
  
  fprintf(f, "graph {\n");
  for (graphint j = 0; j < g->numVertices; j++) {
    for (graphint k = g->edgeStart[j]; k < g->edgeStart[j+1]; k++) {
      fprintf(f, "%6"DFMT" -- %6"DFMT" [weight=%"DFMT"]\n", j, g->endVertex[k], g->intWeight[k]);
    }
  }
  fprintf(f, "}\n");
}

void alloc_edgelist(graphedges * g, graphint NE) {
  if (!g) return;
  
  g->numEdges    = NE;
  g->map_size = (3 * NE) * sizeof(graphint);
  g->startVertex = (graphint*)xmmap(NULL, g->map_size, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANON, 0,0);
  g->endVertex   = &g->startVertex[NE];
  g->intWeight   = &g->endVertex[NE];
}

void free_edgelist(graphedges * g) {
  if (g)
    munmap((caddr_t)g->startVertex, g->map_size);
}

void print_edgelist(graphedges * g, FILE * f) {
  fprintf(f, "edgelist: %"DFMT"\n", g->numEdges);
  
  for (int i=0; i<g->numEdges; i++) {
    fprintf(f, "\t(%6"DFMT", %6"DFMT") : %"DFMT"\n",
            g->startVertex[i], g->endVertex[i], g->intWeight[i]);
  }
}

void print_edgelist_dot(graphedges* g, char* fbase) {
  char fname[512];
  sprintf(fname, "%s%s", fbase, ".edgelist.dot");
  FILE* f = fopen(fname,"w");
  
  fprintf(f, "digraph {\n");
  for (int i=0; i<g->numEdges; i++) {
    fprintf(f, "\t%6"DFMT" -> %6"DFMT" [weight=%"DFMT"];\n",
            g->startVertex[i], g->endVertex[i], g->intWeight[i]);
  }
  fprintf(f, "}\n");
  fclose(f);
}

