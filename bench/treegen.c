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

// arg this needs to be refactored pronto
int main(int argc, char *argv[]) {
  assert(argc >= 6 && argc <= 7);
  double branch = strtod(argv[1], NULL);
  int depth = strtol(argv[2], NULL, 0);
  int minsize = strtol(argv[3], NULL, 0);
  int maxsize = strtol(argv[4], NULL, 0);
  unsigned int seed = 0;
  if (argc == 7) seed = strtol(argv[6], NULL, 0);
  if (seed == 0) seed = goodseed();
  srand(seed);
  int tree_size = -1;
  node *tree = NULL;
  while (tree_size < minsize || tree_size > maxsize) {
    printf(".");
    free_tree(tree);
    tree = make_tree(branch, depth, &tree_size);
  }
  printf("%d\n", tree_size);
  FILE *treefile = fopen(argv[5], "w");
  assert (treefile != NULL);
  write_tree(treefile, tree);
  fclose(treefile);
  free(tree);
  return 0;
}
