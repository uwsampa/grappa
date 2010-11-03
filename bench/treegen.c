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


// uniform random in [0,1)
static double drandom() {
  return rand() * (1.0 / (RAND_MAX + 1.0));
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

// create a tree with v = e + 1 = <size>, rooted at vertex 0. The probability of a vertex having two children is <branch>.
graph *maketree(uint64_t size, double branch) {
  graph *g = graph_new(size, size -1);
  unsigned int index = 0;
  unsigned int next_child = 1;
  for (uint64_t i = 0; i < size; ++i) {
    g->row_ptr[i] = index;
    unsigned int n_children = drandom() < branch ? 2 : 1;
    unsigned int space = size - next_child;
    n_children = n_children > space ? space : n_children;
    for (unsigned int j = 0; j < n_children; ++j) {
      g->edges[index++] = next_child++;
      g->e++;
    }
    g->row_ptr[i+1] = index;
  }
  assert(index == (size - 1));
  assert(next_child == size);
  return g;
}

// arg this needs to be refactored pronto
int main(int argc, char *argv[]) {
  assert(argc >= 4 && argc <= 5);
  double branch = strtod(argv[1], NULL);
  int size = strtol(argv[2], NULL, 0);
  unsigned int seed = 0;
  if (argc == 5) seed = strtol(argv[4], NULL, 0);
  if (seed == 0) seed = goodseed();
  srand(seed);
  graph *g = maketree(size, branch);
  
  FILE *file = fopen(argv[3], "w");
  assert (file != NULL);
  uint64_t *perm = genperm(size);
  // print root
  fprintf(file, "%" PRIu64 "\n", perm[0]);

  /*  for (unsigned int i = 0; i < size; ++i) {
    fprintf(file, "%u -> %" PRIu64 "\n", i, perm[i]);
    }*/
  graph_write(file, g, perm);
  fclose(file);
  graph_free(g);
  free(perm);
  return 0;
}
