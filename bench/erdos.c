#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "graph.h"



static unsigned int goodseed() {
  FILE *urandom = fopen("/dev/urandom", "r");
  unsigned int seed;
  assert(1 == fread(&seed, sizeof(unsigned int), 1, urandom));
  fclose(urandom);

  return seed;
}


// returns uniformly chosen integer in [i,j)
static int rand_range(int i, int j) {
  int spread = j - i;
  int limit = RAND_MAX - RAND_MAX % spread;
  int rnd;
  do {
    rnd = rand();
  } while (rnd >= limit);
  rnd %= spread;
  return (rnd + i);
}


// directed analogue of Erdos random graph (though not quite the same model)
// size*degree total edges, each chosen uniformly at random.
// can contain duplicate edges, self edges...
graph *makegraph(int size, int degree) {
  int edges = size*degree;
  graph *g = graph_new(size, edges);
  g->e = edges;
  for (int i = 0; i < edges; ++i) {
    // destination of edges
    g->edges[i] = rand_range(0, size);
    // which vertex gets another edge -- accumulate degrees
    g->row_ptr[rand_range(0, size)]++;
  }
  int count = 0;
  for (int i = 0; i < size; ++i) {
    int temp = g->row_ptr[i];
    g->row_ptr[i] = count;
    count += temp;
  }
  g->row_ptr[size] = count;
  assert (count == edges);
  return g;
}


int main(int argc, char *argv[]) {
  if (argc != 4) {
    exit(1);
  }
  int degree = strtol(argv[1], NULL, 0);
  int size = strtol(argv[2], NULL, 0);
  int seed = goodseed();
  srand(seed);
  graph *g = makegraph(size, degree);
  FILE *file = fopen(argv[3], "w");
  assert (file != NULL);
  // no need to permute -- totally structure free already
  graph_write(file, g, NULL);
  fclose(file);
  graph_free(g);
  return 0;
}
