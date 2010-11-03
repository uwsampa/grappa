#ifndef GRAPH_H
#define GRAPH_H

#include <inttypes.h>
#include <stdio.h>

// TODO replace this with some standard CSR library
typedef struct graph {
  uint64_t v, e;
  // endpoints of each edge -- size E
  uint64_t *edges;
  // edges from vertex v go to [edges[row_ptr[v]], edges[row_ptr[v+1]])
  // size V + 1
  uint64_t *row_ptr;
} graph;

// Allocate (via malloc) a graph with room for v vertices and e edges.
graph *graph_new(uint64_t v, uint64_t e);

// Delete a graph allocated with graph_new().
void graph_free(graph *g);

// Read a graph in format given by graph_write().
graph *graph_read(FILE *f);

// Write a graph in the following format:
// one line "<v> <e>"
// one line per vertex: an integer count of edges, followed by a space
// separated list of edges, each edge specified by target index.
//
// Example: K_3 looks like this:
// 3 6
// 2 1 2
// 2 0 2
// 2 0 1
//
// If <perm> is non-NULL, it is a permutation to apply to the vertices
// in the output.
void graph_write(FILE *f, graph *g, uint64_t *perm);

#endif
