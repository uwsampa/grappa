#include "graph.h"

#include <assert.h>
#include <stdlib.h>

// Allocate (via malloc) a graph with v vertices and space for e edges.
graph *graph_new(uint64_t v, uint64_t e) {
  graph *g = malloc(sizeof(graph));
  assert(g != NULL);
  g->edges = calloc(sizeof(uint64_t), e);
  assert(g->edges != NULL);
  g->row_ptr = calloc(sizeof(uint64_t), v+1);
  assert(g->row_ptr != NULL);
  g->v = v;
  g->e = 0;
  return g;
}

void graph_free(graph *g) {
  free(g->edges);
  free(g->row_ptr);
  free(g);
}

graph *graph_read(FILE *f) {
  uint64_t v,e;
  fscanf(f, "%" PRIu64 " %" PRIu64 "", &v, &e);
  graph *g = graph_new(v, e);
  unsigned int next = 0;
  for (unsigned int i = 0; i < v; ++i) {
    g->row_ptr[i] = next;
    uint64_t degree;
    fscanf(f, "%" PRIu64 "", &degree);
    for (unsigned int j = 0; j < degree; ++j) {
      fscanf(f, "%" PRIu64 "", &g->edges[next++]);
    }
  }
  g->row_ptr[v] = next;
  assert(next == e);
  return g;
}

void graph_write(FILE *f, graph *g, uint64_t *perm) {
  fprintf(f, "%" PRIu64 " %" PRIu64 "\n", g->v, g->e);
  for (unsigned int j = 0; j < g->v; ++j) {
    unsigned int v = perm != NULL ? perm[j] : j;
    uint64_t degree = g->row_ptr[v+1] - g->row_ptr[v];
    uint64_t index = g->row_ptr[v];
    fprintf(f, "%" PRIu64 "", degree);
    for (unsigned int j = 0; j < degree; ++j) {
      uint64_t edge = g->edges[index++];
      edge = perm != NULL ? perm[edge] : edge;
      fprintf(f, " %" PRIu64 "", edge);
    }
    fprintf(f, "\n");
  }
}
