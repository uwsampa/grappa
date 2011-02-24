#include "graph.h"

#include <assert.h>
#include <stdlib.h>

static uint64_t *invert_perm(uint64_t *perm, unsigned int n) {
  if (perm == NULL) return NULL;
  uint64_t *inv = calloc(sizeof(uint64_t), n);
  assert(inv != NULL);

  for (unsigned int i = 0; i < n; ++i) {
    int j = perm[i];
    inv[j] = i;
  }
  return inv;
}

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
  assert(2 == fscanf(f, "%" PRIu64 " %" PRIu64 "", &v, &e));
  graph *g = graph_new(v, e);
  unsigned int next = 0;
  for (unsigned int i = 0; i < v; ++i) {
    if (i % 10000 == 0) printf("Reading vertex %d...\n", i);
    g->row_ptr[i] = next;
    uint64_t degree;
    assert(1 == fscanf(f, "%" PRIu64 "", &degree));
    for (unsigned int j = 0; j < degree; ++j) {
      g->e++;
      assert(1 == fscanf(f, "%" PRIu64 "", &g->edges[next++]));
    }
  }
  g->row_ptr[v] = next;
  assert(next == e);
  assert(e == g->e);
  printf("Done.\n");
  return g;
}

void graph_write(FILE *f, graph *g, uint64_t *perm) {
  uint64_t *inv = invert_perm(perm, g->v);
  fprintf(f, "%" PRIu64 " %" PRIu64 "\n", g->v, g->e);
  for (unsigned int i = 0; i < g->v; ++i) {
    unsigned int v = inv != NULL ? inv[i] : i;
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
  free(inv);
}

int graph_edgecount(graph *g, uint64_t u, uint64_t v) {
  int num = 0;
  uint64_t from = g->row_ptr[u];
  uint64_t to = g->row_ptr[u+1];
  for (; from < to; ++from) {
    if (g->edges[from] == v) num++;
  }
  return num;
}
