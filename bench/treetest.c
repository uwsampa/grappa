#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tree.h"

static unsigned int goodseed() {
  FILE *urandom = fopen("/dev/urandom", "r");
  unsigned int seed;
  assert(1 == fread(&seed, sizeof(unsigned int), 1, urandom));
  fclose(urandom);

  return seed;
}

int main(int argc, char *argv[]) {
  assert(argc >= 3 && argc <= 4);
  double branch = strtod(argv[1], NULL);
  int depth = strtol(argv[2], NULL, 0);
  unsigned int seed = 0;
  if (argc == 4) seed = strtol(argv[3], NULL, 0);
  if (seed == 0) seed = goodseed();
  srand(seed);
  int tree_size = -1;
  node *tree = make_tree(branch, depth, &tree_size);
  free_tree(tree);
  printf("%d\n", tree_size);

  return 0;
}
