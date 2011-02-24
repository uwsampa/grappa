#include <assert.h>
#include <math.h>
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

// uniform random in [0,1)
static double drandom() {
  return rand() * (1.0 / (RAND_MAX + 1.0));
}


// Graph500 generator:
// www.graph500.org/Specifications.html
// A few differences (for simplicity):
// * haven't bothered making it undirected
graph *makegraph(int scale, int edgefactor) {
  int n = pow(2, scale);
  int m = edgefactor*n;
  double a = 0.57, b = 0.19, c = 0.19;
  double ab = a+b;
  double c_norm = c/(1-ab);
  double a_norm = a/ab;
  graph *g = graph_new(n, m);
  uint64_t *froms = calloc(sizeof(uint64_t), m);
  uint64_t *tos = calloc(sizeof(uint64_t), m);
  for (int i = 0; i < m; ++i) {
    int from = 0, to = 0;
    for (int b = 0; b < scale; ++b) {
      int fac = pow(2, b);
      int ibit = drandom() > ab ? 1 : 0;
      int jbit = drandom() > (c_norm*ibit+a_norm*(1-ibit)) ? 1 : 0;
      from += ibit*fac;
      to += jbit*fac;
    }
    froms[i] = from;
    tos[i] = to;
    g->row_ptr[from]++;
  }

  int sum = 0;
  for (int i = 0; i < n; ++i) {
    int s = g->row_ptr[i];
    g->row_ptr[i] = sum;
    sum += s;
  }
  g->row_ptr[n] = sum;
  assert (sum == m);
  uint64_t *counts = calloc(sizeof(uint64_t), n);
  for (int i  = 0; i < m; ++i) {
    g->e++;
    int from = froms[i];
    int to = tos[i];
    int e = g->row_ptr[from]+counts[from]++;
    // printf("(%d, %d) to %d\n", from, to, e);
    g->edges[e] = to;
  }

  free(froms);
  free(tos);
  free(counts);
  return g;
}


uint64_t *genperm(int n) {
  assert(n <= RAND_MAX);
  uint64_t *perm = calloc(sizeof(uint64_t), n);
  assert(perm != NULL);
  for (int i = 0; i < n; ++i) {
    perm[i] = i;
  }
  for (int i = n - 1; i > 0; --i) {
    int j = rand_range(0, i+1);
    uint64_t temp = perm[i];
    perm[i] = perm[j];
    perm[j] = temp;
  }
  return perm;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    exit(1);
  }
  int scale = strtol(argv[1], NULL, 0);
  int edgefactor = strtol(argv[2], NULL, 0);
  int seed = goodseed();
  srand(seed);
  graph *g = makegraph(scale, edgefactor);
  uint64_t *perm = genperm(pow(2, scale));
  FILE *file = fopen(argv[3], "w");
  assert (file != NULL);
  graph_write(file, g, perm);
  fclose(file);
  graph_free(g);
  free(perm);
  return 0;
}
