#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timeb.h>
#include "graph.h"

static uint64_t get_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t ns = 0;
  ns += ts.tv_nsec;
  ns += ts.tv_sec * (1000LL * 1000LL * 1000LL);
  return ns;
}

uint64_t basic_depth(graph *tree, uint64_t root,
                     uint64_t *to_examine, uint64_t *next) {
  unsigned int to_examine_size = 1;
  to_examine[0] = root;
  uint64_t depth = 0;
  while (to_examine_size > 0) {
    depth++;
    unsigned int next_size = 0;
    for (unsigned int i = 0; i < to_examine_size; ++i) {
      uint64_t vertex = to_examine[i];
      for (unsigned int index = tree->row_ptr[vertex];
           index < tree->row_ptr[vertex+1]; ++index) {
        next[next_size++] = tree->edges[index];
      }
    }

    uint64_t *temp = to_examine;
    to_examine = next;
    next = temp;
    to_examine_size = next_size;
  }
  return depth;
}

int main(int argc, char *argv[]) {
  assert(argc == 2);

  FILE *treefile = fopen(argv[1], "r");
  assert (treefile != NULL);

  uint64_t root;
  assert(1 == fscanf(treefile, "%" PRIu64 "\n", &root));

  graph *tree = graph_read(treefile);
  fclose(treefile);
  uint64_t *to_examine = calloc(sizeof(uint64_t), tree->v);
  uint64_t *next = calloc(sizeof(uint64_t), tree->v);
  for (unsigned int i = 0; i < tree->v; ++i) {
    to_examine[i] = next[i] = i;
  }

  for (int i = 0; i < 10; ++i) {
    uint64_t before = get_ns();
    uint64_t answer = basic_depth(tree, root, to_examine, next);
    uint64_t after = get_ns();

    uint64_t elapsed = after - before;
    double rate = elapsed;
    rate /= tree->v;

    printf("depth is %" PRIu64 "\n", answer);
    printf("%" PRIu64 " ns -> %f ns/vertex \n", elapsed, rate);
  }
  free(to_examine);
  free(next);

  return 0;
}
